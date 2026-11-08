#ifndef LAB_2
#define LAB_2

#include "bmi323.h"
#include <stdint.h>

// Things to add to the Watch panel
extern uint8_t lab_2_found[8];       // addresses that ACKed in the bus scan
extern uint8_t lab_2_found_count;
extern uint16_t lab_2_chip_id;       // CHIP_ID register
extern bmi323_reading_t lab_2_imu;   // latest sample
extern float lab_2_roll, lab_2_pitch; // tilt in degrees

// Main lab_2 loop
void lab_2(void);

#endif
