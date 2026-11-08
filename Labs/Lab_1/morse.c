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

  // TODO (part 4): remember when this state started, set duration from the
  // current character (dot or dash), and turn the LED on.
  (void)ctx;
}

// LED off for the gap after a symbol: 1 unit inside a letter, 3 units between
// letters (a ' ' in the message), 7 units before the message repeats
static void on_enter_off(fsm_t *fsm, void *context) {
  morse_context_t *ctx = (morse_context_t *)context;

  // TODO (part 4): move ctx->index to the next dot or dash (skip a space, wrap
  // around at the end of the string), pick the gap length, remember when
  // this state started, and turn the LED off.
  (void)ctx;
}

// Transition predicate: has the current state lasted long enough?
static fsm_bool state_time_up(fsm_t *fsm, void *context) {
  morse_context_t *ctx = (morse_context_t *)context;

  // TODO (part 4)
  (void)ctx;
  return false;
}

void morse_start(void) {
  morse_context_t ctx = {.message = MORSE_MESSAGE};

  morse_fsm = fsm_create(malloc, free, &ctx, sizeof(ctx));

  // The first state added is the one the FSM starts in
  fsm_add_state(morse_fsm, (fsm_state_t){.name = "On", .on_enter = on_enter_on});
  fsm_add_state(morse_fsm, (fsm_state_t){.name = "Off", .on_enter = on_enter_off});

  // TODO (part 4): add the two transitions, On -> Off and Off -> On, both
  // taken when state_time_up says so. See c-fsm/examples/basic.c.
  (void)state_time_up;
}

void morse_update(void) { fsm_run(morse_fsm); }
