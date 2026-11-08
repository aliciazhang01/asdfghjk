#ifndef MORSE
#define MORSE

#include <stdint.h>

// Length of one Morse "unit" (one dot). Everything else is a multiple of it.
#define MORSE_UNIT_MS 150

// The message as dots and dashes, letters separated by one space.
// "NFR" = -. ..-. .-.
#define MORSE_MESSAGE "-. ..-. .-."

// Part 4: builds the state machine. Call once.
void morse_start(void);

// Part 4: call as often as possible. Runs one step of the state machine.
void morse_update(void);

#endif
