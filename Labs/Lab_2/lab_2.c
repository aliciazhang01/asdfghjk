#include "lab_2.h"
#include "bmi323.h"
#include "i2c.h"
#include "main.h"
#include <math.h>
#include <stdio.h>

// Which part of the lab runs. Change this, rebuild, and flash.
#define LAB_2_PART 1

#define RAD_TO_DEG (180.0f / 3.14159265f)
#define LEVEL_DEG 3.0f // closer to flat than this counts as level

uint8_t lab_2_found[8];
uint8_t lab_2_found_count;
uint16_t lab_2_chip_id;
bmi323_reading_t lab_2_imu;
float lab_2_roll, lab_2_pitch;

// ---------------------------------------------------------------- part 1

// Probes every valid 7-bit address and records the ones that ACK
static void scan_bus(void) {
  lab_2_found_count = 0;

  // TODO (part 1): valid 7-bit addresses go from 0x08 to 0x77. Probe each
  // one with i2c_probe, and add the ones that answer to lab_2_found (don't
  // overflow it).

  printf("I2C: scan done, %u found\r\n", lab_2_found_count);
}

// ---------------------------------------------------------------- part 2

static void read_chip_id(void) {
  // TODO (part 2): use the address part 1 found
  bmi323_addr = 0;

  if (bmi323_read_regs(BMI323_REG_CHIP_ID, &lab_2_chip_id, 1) != 0) {
    printf("IMU: no answer at 0x%02x\r\n", bmi323_addr);
    return;
  }
  printf("IMU: CHIP_ID = 0x%04x\r\n", lab_2_chip_id);
}

// ---------------------------------------------------------------- part 4

// Tilt from the direction of gravity. Only valid while the board isn't
// accelerating.
static void compute_tilt(const bmi323_reading_t *r) {
  float ax = r->acc_g[0], ay = r->acc_g[1], az = r->acc_g[2];

  // TODO (part 4): roll is rotation about X, pitch is rotation about Y.
  // Lying flat, gravity is all on Z and both should be 0. Use atan2f (it
  // returns radians) and the handout's diagram.
  (void)ax;
  (void)ay;
  (void)az;
  lab_2_roll = 0;
  lab_2_pitch = 0;
}

// LED solid when level, otherwise blinks faster the more the board tilts
static void show_level(void) {
  float tilt = fmaxf(fabsf(lab_2_roll), fabsf(lab_2_pitch));

  // TODO (part 4): if tilt is under LEVEL_DEG, turn the LED on. Otherwise
  // blink it without blocking (like blink_nonblocking in lab 1), with a
  // period that gets shorter as the tilt gets bigger.
  (void)tilt;
}

// ---------------------------------------------------------------- main

void lab_2(void) {
  printf("lab 2 part %d\r\n", LAB_2_PART);

  switch (LAB_2_PART) {
  case 1: // who's on the bus?
    scan_bus();
    while (1) {
    }

  case 2: // read one register
    read_chip_id();
    while (1) {
    }

  case 3: // stream readings
  case 4: // level indicator
    if (bmi323_init() != 0) {
      printf("IMU: init failed\r\n");
      while (1) {
      }
    }
    printf("IMU: ready at 0x%02x\r\n", bmi323_addr);

    while (1) {
      if (bmi323_read(&lab_2_imu) != 0) {
        continue;
      }

      if (LAB_2_PART == 3) {
        // printf can't do floats here, so print whole milli-units
        printf("acc %5d %5d %5d mg | gyr %6d %6d %6d mdps\r\n",
               (int)(lab_2_imu.acc_g[0] * 1000),
               (int)(lab_2_imu.acc_g[1] * 1000),
               (int)(lab_2_imu.acc_g[2] * 1000),
               (int)(lab_2_imu.gyr_dps[0] * 1000),
               (int)(lab_2_imu.gyr_dps[1] * 1000),
               (int)(lab_2_imu.gyr_dps[2] * 1000));
        HAL_Delay(100);
      } else {
        compute_tilt(&lab_2_imu);
        show_level();
      }
    }
  }
}
