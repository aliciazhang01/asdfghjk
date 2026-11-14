# c-fsm

A **simple, single-header** implementation of a finite state machine in C.

## Running Examples

There are a few examples in the `examples` directory. To build them, run the following command:

```bash
./build_examplessh
```

This will create a build directory with all the examples compiled. To run them run the following command:

```bash
./run_examples.sh
```

This will check for the executables in the build directory and ask you which one you want to run.

Alternatively, you can run the examples manually by running the following command:

```bash
./build/<example_name>.exe
```

## Adding to Your Project
Just copy the `fsm.h` file to your project and include it in your source files.

Then, in one of your source files, define the implementation of the FSM:

```c
#define FSM_IMPL
#include "fsm.h"
```

This will include the implementation of the FSM in that source file. You can include the header file in as many source files as you want, but you should only define the implementation in one of them. This practice is taken from the STB libraries, and it is probably the easiest way to develop a simple library like this.

## Usage Example
Documentation is still underway, but here is a simple example of how to use the FSM:

```c
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

    // just wait a sec, just to make the example more interesting
    // in practice, you wouldn't do this
    // very stupid way to do this, but it's just an example
    for (int i = 0; i < 1000000000; i++) {
      asm("nop");
    }

  }

  fsm_destroy(g_fsm);

  return 0;
}
```