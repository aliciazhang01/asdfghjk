#ifndef BMI323_H
#define BMI323_H

#include <stdint.h>

// BMI323 6-axis IMU (U4) on I2C1. Register numbers and bit fields are from
// the BMI323 datasheet (link in the top-level README).

#define BMI323_ADDR_SDO_LOW 0x68
#define BMI323_ADDR_SDO_HIGH 0x69
#define BMI323_CHIP_ID 0x43 // low byte of the CHIP_ID register

#define BMI323_REG_CHIP_ID 0x00
#define BMI323_REG_ERR 0x01
#define BMI323_REG_STATUS 0x02
#define BMI323_REG_ACC_X 0x03 // ACC X/Y/Z, GYR X/Y/Z, TEMP follow in order
#define BMI323_REG_ACC_CONF 0x20
#define BMI323_REG_GYR_CONF 0x21
#define BMI323_REG_CMD 0x7E

#define BMI323_CMD_SOFT_RESET 0xDEAF

typedef struct {
  int16_t acc_raw[3]; // X/Y/Z straight from the chip
  int16_t gyr_raw[3];
  int16_t temp_raw;

  float acc_g[3];   // acceleration in g
  float gyr_dps[3]; // rotation rate in degrees per second
  float temp_c;     // die temperature in degrees C
} bmi323_reading_t;

// The I2C address the chip answered on (set by bmi323_init)
extern uint8_t bmi323_addr;

// Reads count 16-bit registers starting at reg into values
int bmi323_read_regs(uint8_t reg, uint16_t *values, uint8_t count);

// Writes one 16-bit register
int bmi323_write_reg(uint8_t reg, uint16_t value);

// Finds the chip, resets it and turns on the accelerometer and gyroscope.
// Returns 0 on success.
int bmi323_init(void);

// Reads one sample of everything and converts it to physical units.
// Returns 0 on success.
int bmi323_read(bmi323_reading_t *reading);

#endif
