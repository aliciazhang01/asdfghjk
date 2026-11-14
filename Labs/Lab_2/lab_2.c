#include "lab_2.h"
#include "bmi323.h"
#include "i2c.h"
#include "main.h"
#include <math.h>
#include <stdio.h>

// Which part of the lab runs. Change this, rebuild, and flash.
#ifndef LAB_2_PART
#define LAB_2_PART 4
#endif

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

  for (uint8_t i = 0; i < sizeof(lab_2_found); ++i) {
    lab_2_found[i] = 0;
  }
  for (uint8_t address = 0x08; address <= 0x77; ++address) {
    if (i2c_probe(address) == 0 && lab_2_found_count < sizeof(lab_2_found)) {
      lab_2_found[lab_2_found_count++] = address;
    }
  }

  printf("I2C: scan done, %u found\r\n", lab_2_found_count);
}

// ---------------------------------------------------------------- part 2

static void read_chip_id(void) {
  scan_bus();
  lab_2_chip_id = 0;
  bmi323_addr = 0;
  for (uint8_t i = 0; i < lab_2_found_count; ++i) {
    uint8_t address = lab_2_found[i];
    if (address != BMI323_ADDR_SDO_LOW && address != BMI323_ADDR_SDO_HIGH) {
      continue;
    }
    bmi323_addr = address;
    if (bmi323_read_regs(BMI323_REG_CHIP_ID, &lab_2_chip_id, 1) == 0 &&
        (lab_2_chip_id & 0xFF) == BMI323_CHIP_ID) {
      printf("IMU: address 0x%02x, CHIP_ID = 0x%04x\r\n",
             bmi323_addr, lab_2_chip_id);
      return;
    }
  }
  printf("IMU: BMI323 not found at 0x68 or 0x69\r\n");
}

// ---------------------------------------------------------------- part 4

// Tilt from the direction of gravity. Only valid while the board isn't
// accelerating.
static void compute_tilt(const bmi323_reading_t *r) {
  float ax = r->acc_g[0], ay = r->acc_g[1], az = r->acc_g[2];

  lab_2_roll = atan2f(ay, az) * RAD_TO_DEG;
  lab_2_pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG;
}

// Use elapsed time rather than delays so sensor sampling keeps running.
static void show_level(void) {
  static uint32_t last_toggle = 0;
  uint32_t now = HAL_GetTick();
  float tilt = fmaxf(fabsf(lab_2_roll), fabsf(lab_2_pitch));
  if (tilt <= LEVEL_DEG) {
    HAL_GPIO_WritePin(Status_LED_GPIO_Port, Status_LED_Pin, GPIO_PIN_SET);
    last_toggle = now;
    return;
  }
  uint32_t interval = (uint32_t)(500.0f / (1.0f + tilt / 10.0f));
  if (interval < 50U) {
    interval = 50U;
  }
  if ((uint32_t)(now - last_toggle) >= interval) {
    HAL_GPIO_TogglePin(Status_LED_GPIO_Port, Status_LED_Pin);
    last_toggle = now;
  }
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
        printf("acc %5d %5d %5d mg | gyr %6d %6d %6d mdps | temp %d mC\r\n",
               (int)(lab_2_imu.acc_g[0] * 1000),
               (int)(lab_2_imu.acc_g[1] * 1000),
               (int)(lab_2_imu.acc_g[2] * 1000),
               (int)(lab_2_imu.gyr_dps[0] * 1000),
               (int)(lab_2_imu.gyr_dps[1] * 1000),
               (int)(lab_2_imu.gyr_dps[2] * 1000),
               (int)(lab_2_imu.temp_c * 1000));
        HAL_Delay(100);
      } else {
        compute_tilt(&lab_2_imu);
        show_level();
      }
    }
  }
}
