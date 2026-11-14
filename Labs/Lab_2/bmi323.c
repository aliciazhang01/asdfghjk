#include "bmi323.h"
#include "i2c.h"
#include "main.h"
#include <stddef.h>

// BMI323 ACC_CONF/GYR_CONF: 100 Hz, +/-8 g, +/-2000 dps, normal mode.
#define CONF_ODR_100HZ 0x8U
#define CONF_ACC_RANGE_8G (0x2U << 4)
#define CONF_GYR_RANGE_2000DPS (0x4U << 4)
#define CONF_MODE_NORMAL (0x4U << 12)

#define ACC_CONF (CONF_MODE_NORMAL | CONF_ACC_RANGE_8G | CONF_ODR_100HZ)
#define GYR_CONF (CONF_MODE_NORMAL | CONF_GYR_RANGE_2000DPS | CONF_ODR_100HZ)

// Signed 16-bit full scale: 32768 / range. Temperature: raw / 512 + 23.
#define ACC_LSB_PER_G 4096.0f
#define GYR_LSB_PER_DPS 16.384f
#define TEMP_LSB_PER_C 512.0f
#define TEMP_OFFSET_C 23.0f

#define ERR_CONF 0x0060 // ERR_REG: acc_conf_err | gyr_conf_err
#define NO_DATA ((int16_t)0x8000) // value read before the first sample

#define MAX_READ 7 // ACC X/Y/Z + GYR X/Y/Z + TEMP

uint8_t bmi323_addr;

int bmi323_read_regs(uint8_t reg, uint16_t *values, uint8_t count) {
  uint8_t data[2 + 2 * MAX_READ];
  if (values == NULL || count == 0 || count > MAX_READ) {
    return -1;
  }
  if (i2c_read_reg(bmi323_addr, reg, data, (uint8_t)(2 + 2 * count)) != 0) {
    return -1;
  }
  // Discard the two dummy bytes, then decode little-endian register words.
  for (uint8_t i = 0; i < count; ++i) {
    values[i] = (uint16_t)data[2 + 2 * i] |
                ((uint16_t)data[3 + 2 * i] << 8);
  }
  return 0;
}

int bmi323_write_reg(uint8_t reg, uint16_t value) {
  const uint8_t data[2] = {(uint8_t)value, (uint8_t)(value >> 8)};
  return i2c_write_reg(bmi323_addr, reg, data, sizeof(data));
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

  if (reading == NULL ||
      bmi323_read_regs(BMI323_REG_ACC_X, raw, MAX_READ) != 0) {
    return -1;
  }
  if ((int16_t)raw[0] == NO_DATA || (int16_t)raw[3] == NO_DATA ||
      (int16_t)raw[6] == NO_DATA) {
    return -1;
  }

  for (uint8_t i = 0; i < 3; ++i) {
    reading->acc_raw[i] = (int16_t)raw[i];
    reading->gyr_raw[i] = (int16_t)raw[i + 3];
    reading->acc_g[i] = reading->acc_raw[i] / ACC_LSB_PER_G;
    reading->gyr_dps[i] = reading->gyr_raw[i] / GYR_LSB_PER_DPS;
  }
  reading->temp_raw = (int16_t)raw[6];
  reading->temp_c = reading->temp_raw / TEMP_LSB_PER_C + TEMP_OFFSET_C;
  return 0;
}
