
#ifndef BLINKY
#define BLINKY

#include <stdint.h>

#define BLINK_DURATION_DEFAULT 500 // ms the LED stays on, then off

// Part 1: blinks once with HAL_Delay. The CPU is stuck here for the whole
// blink.
void blink_blocking();

// Part 2: call as often as possible. Toggles the LED whenever
// BLINK_DURATION_DEFAULT ms have passed, otherwise returns right away.
void blink_nonblocking();

// Part 3: sets up TIM1 so its update interrupt fires toggle_hz times a second,
// then starts it. HAL_TIM_PeriodElapsedCallback toggles the LED.
void blink_timer_start(uint32_t toggle_hz);

#endif
