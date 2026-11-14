#include "morse.h"
#include "main.h"
#include <stdlib.h>

#define FSM_IMPL
#include "fsm.h"

// Everything the two states share
typedef struct {
  const char *message; // dots, dashes and spaces, e.g. "-. ..-. .-."
  uint32_t index;      // which character of message is playing
  uint32_t entered_at; // HAL_GetTick() when the current state started
  uint32_t duration;   // how long the current state lasts, in ms
} morse_context_t;

static fsm_t *morse_fsm;

// LED on for one dot (1 unit) or one dash (3 units)
static void on_enter_on(fsm_t *fsm, void *context) {
  morse_context_t *ctx = (morse_context_t *)context;

  (void)fsm;

  ctx->entered_at = HAL_GetTick();

  // '-' is a dash (3 units), anything else is treated as a dot (1 unit)
  if (ctx->message[ctx->index] == '-') {
    ctx->duration = 3 * MORSE_UNIT_MS;
  } else {
    ctx->duration = 1 * MORSE_UNIT_MS;
  }

  HAL_GPIO_WritePin(Status_LED_GPIO_Port, Status_LED_Pin, GPIO_PIN_SET);
}

// LED off for the gap after a symbol: 1 unit inside a letter, 3 units between
// letters (a ' ' in the message), 7 units before the message repeats
static void on_enter_off(fsm_t *fsm, void *context) {
  morse_context_t *ctx = (morse_context_t *)context;

  (void)fsm;

  // Look at what comes after the symbol that just played
  uint32_t next = ctx->index + 1;
  uint32_t gap_units = 1; // default: next symbol is in the same letter

  if (ctx->message[next] == ' ') {
    gap_units = 3; // end of a letter
    while (ctx->message[next] == ' ') {
      next++; // skip the space(s)
    }
  }

  if (ctx->message[next] == '\0') {
    gap_units = 7; // end of the whole message, start over
    next = 0;
  }

  ctx->index = next;
  ctx->duration = gap_units * MORSE_UNIT_MS;
  ctx->entered_at = HAL_GetTick();

  HAL_GPIO_WritePin(Status_LED_GPIO_Port, Status_LED_Pin, GPIO_PIN_RESET);
}

// Transition predicate: has the current state lasted long enough?
static fsm_bool state_time_up(fsm_t *fsm, void *context) {
  morse_context_t *ctx = (morse_context_t *)context;

  (void)fsm;

  // Same wraparound-safe comparison as blink_nonblocking()
  return (HAL_GetTick() - ctx->entered_at) >= ctx->duration;
}

void morse_start(void) {
  morse_context_t ctx = {.message = MORSE_MESSAGE};

  morse_fsm = fsm_create(malloc, free, &ctx, sizeof(ctx));

  // The first state added is the one the FSM starts in
  fsm_add_state(morse_fsm, (fsm_state_t){.name = "On", .on_enter = on_enter_on});
  fsm_add_state(morse_fsm, (fsm_state_t){.name = "Off", .on_enter = on_enter_off});

  // Both transitions use the same predicate: leave the state once its
  // duration (set in that state's on_enter) has passed.
  fsm_add_transition(morse_fsm, "On", "Off", FSM_PREDICATE_GROUP(state_time_up));
  fsm_add_transition(morse_fsm, "Off", "On", FSM_PREDICATE_GROUP(state_time_up));
}

void morse_update(void) { fsm_run(morse_fsm); }
