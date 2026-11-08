#include "bmi323.h"
#include "i2c.h"
#include "main.h"

// ACC_CONF / GYR_CONF bit fields.
// TODO (part 3): fill these in from the ACC_CONF and GYR_CONF register
// descriptions in the datasheet. Shift each value to its bit position.
#define CONF_ODR_100HZ 0x0         // bits 3:0, 100 Hz output data rate
#define CONF_ACC_RANGE_8G 0x0      // bits 6:4, +/-8 g
#define CONF_GYR_RANGE_2000DPS 0x0 // bits 6:4, +/-2000 dps
#define CONF_MODE_NORMAL 0x0       // bits 14:12, normal (continuous) mode

#define ACC_CONF (CONF_MODE_NORMAL | CONF_ACC_RANGE_8G | CONF_ODR_100HZ)
#define GYR_CONF (CONF_MODE_NORMAL | CONF_GYR_RANGE_2000DPS | CONF_ODR_100HZ)

// Sensitivity for the ranges above.
// TODO (part 3): the raw value is a signed 16-bit number that spans the whole
// range, so +32767 is about +8 g. Work out how many counts are one g and one
// degree per second. The temperature formula is in the datasheet.
#define ACC_LSB_PER_G 1.0f
#define GYR_LSB_PER_DPS 1.0f
#define TEMP_LSB_PER_C 1.0f
#define TEMP_OFFSET_C 0.0f

#define ERR_CONF 0x0060 // ERR_REG: acc_conf_err | gyr_conf_err
#define NO_DATA ((int16_t)0x8000) // value read before the first sample

#define MAX_READ 7 // ACC X/Y/Z + GYR X/Y/Z + TEMP

uint8_t bmi323_addr;

int bmi323_read_regs(uint8_t reg, uint16_t *values, uint8_t count) {
  // Every I2C read returns 2 dummy bytes first, then each register low byte
  // first
  uint8_t data[2 + 2 * MAX_READ];

  if (count > MAX_READ) {
    return -1;
  }

  // TODO (part 2): read 2 + 2 * count bytes starting at reg with
  // i2c_read_reg (return -1 if it fails), skip the 2 dummy bytes, and combine
  // each pair of bytes into values[i]
  (void)data;
  return -1;
}

int bmi323_write_reg(uint8_t reg, uint16_t value) {
  // TODO (part 3): split value into 2 bytes, low byte first, and send them
  // with i2c_write_reg
  return -1;
}

// Tries both possible addresses and keeps the one with the right chip ID
static int bmi323_find(void) {
  const uint8_t addrs[] = {BMI323_ADDR_SDO_LOW, BMI323_ADDR_SDO_HIGH};
  uint16_t id;

  for (uint32_t i = 0; i < sizeof(addrs); i++) {
    bmi323_addr = addrs[i];
    if (bmi323_read_regs(BMI323_REG_CHIP_ID, &id, 1) == 0 &&
        (id & 0xFF) == BMI323_CHIP_ID) {
      return 0;
    }
  }
  return -1;
}

int bmi323_init(void) {
  uint16_t err = 0;

  if (bmi323_find() != 0) {
    return -1;
  }

  if (bmi323_write_reg(BMI323_REG_CMD, BMI323_CMD_SOFT_RESET) != 0) {
    return -1;
  }
  HAL_Delay(2);

  if (bmi323_write_reg(BMI323_REG_ACC_CONF, ACC_CONF) != 0 ||
      bmi323_write_reg(BMI323_REG_GYR_CONF, GYR_CONF) != 0) {
    return -1;
  }
  HAL_Delay(50); // the first samples arrive after a few output periods

  if (bmi323_read_regs(BMI323_REG_ERR, &err, 1) != 0 || (err & ERR_CONF)) {
    return -1;
  }
  return 0;
}

int bmi323_read(bmi323_reading_t *reading) {
  uint16_t raw[MAX_READ];

  if (bmi323_read_regs(BMI323_REG_ACC_X, raw, MAX_READ) != 0) {
    return -1;
  }
  if ((int16_t)raw[0] == NO_DATA || (int16_t)raw[3] == NO_DATA) {
    return -1;
  }

  // raw[0..2] = ACC X/Y/Z, raw[3..5] = GYR X/Y/Z, raw[6] = TEMP
  //
  // TODO (part 3): fill in every field of *reading. The registers are signed
  // (two's complement), so cast to int16_t before converting to units.
  (void)reading;
  return 0;
}
