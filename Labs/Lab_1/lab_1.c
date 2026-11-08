
#include "lab_1.h"
#include "blinky.h"
#include "morse.h"
#include <stdio.h>

// Which part of the lab runs. Change this, rebuild, and flash.
#define LAB_1_PART 1

// How many times the main loop has run. Add it to the Watch panel and compare
// it between parts.
volatile uint32_t lab_1_loop_count = 0;

void lab_1() {
  printf("lab 1 part %d\r\n", LAB_1_PART);

  switch (LAB_1_PART) {
  case 1: // blocking blink
    while (1) {
      blink_blocking();
      lab_1_loop_count++;
    }

  case 2: // non-blocking blink
    while (1) {
      blink_nonblocking();
      lab_1_loop_count++;
    }

  case 3: // timer interrupt blink, 2 toggles a second = 1 Hz blink
    blink_timer_start(2);
    while (1) {
      lab_1_loop_count++;
    }

  case 4: // Morse code with a state machine
    morse_start();
    while (1) {
      morse_update();
      lab_1_loop_count++;
    }
  }
}
