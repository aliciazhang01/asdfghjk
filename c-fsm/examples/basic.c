#include <stdio.h>

#define FSM_IMPL
#include "fsm.h"

fsm_t *g_fsm;

#define STAMINA_THRESHOLD 10

typedef struct fsm_context {
  // something something
  int stamina;
} fsm_context_t;

void idle_on_enter(fsm_t *fsm, void *context) { printf("[idle] Enter!\n"); }

void idle_on_update(fsm_t *fsm, void *context) {
  printf("[idle] Update! Stamina: %d\n", ((fsm_context_t *)context)->stamina);
  ((fsm_context_t *)context)->stamina++;
}

void idle_on_exit(fsm_t *fsm, void *context) { printf("[idle] Exit!\n"); }

void walk_on_enter(fsm_t *fsm, void *context) { printf("[walk] Enter!\n"); }

void walk_on_update(fsm_t *fsm, void *context) {
  printf("[walk] Update! Stamina: %d\n", ((fsm_context_t *)context)->stamina);
  ((fsm_context_t *)context)->stamina--;
}

void walk_on_exit(fsm_t *fsm, void *context) { printf("[walk] Exit!\n"); }

fsm_bool transition_walk_to_idle(fsm_t *fsm, void *context) {
  return ((fsm_context_t *)context)->stamina == 0;
}

fsm_bool transition_idle_to_walk(fsm_t *fsm, void *context) {
  return ((fsm_context_t *)context)->stamina >= STAMINA_THRESHOLD;
}

int main() {
  fsm_context_t context = {0};
  g_fsm = FSM_CREATE(&context);

  fsm_add_state(g_fsm, (fsm_state_t){.name = "Idle",
                                     .on_enter = idle_on_enter,
                                     .on_update = idle_on_update,
                                     .on_exit = idle_on_exit});

  fsm_add_state(g_fsm, (fsm_state_t){.name = "Walk",
                                     .on_enter = walk_on_enter,
                                     .on_update = walk_on_update,
                                     .on_exit = walk_on_exit});

  fsm_add_transition(g_fsm, "Idle", "Walk",
                     FSM_PREDICATE_GROUP(transition_idle_to_walk));

  fsm_add_transition(g_fsm, "Walk", "Idle",
                     FSM_PREDICATE_GROUP(transition_walk_to_idle));

  fsm_set_state(g_fsm, "Idle");

  while (true) {
    fsm_run(g_fsm);

    // just wait a sec
    // very stupid way to do this, but it's just an example
    for (int i = 0; i < 1000000000; i++) {
      asm("nop");
    }
  }

  fsm_destroy(g_fsm);

  return 0;
}