#include "i2c.h"
#include "main.h"

// These functions drive the I2C registers directly so each step of a
// transaction is visible. HAL_I2C_Mem_Read/Write do the same thing. Pin and
// timing setup come from CubeMX. Register names are from the I2C chapter of
// RM0456.

extern I2C_HandleTypeDef hi2c1;

#define I2C (hi2c1.Instance)
#define I2C_TIMEOUT_MS 5

#define I2C_WRITE 0
#define I2C_READ 1
#define I2C_KEEP_BUS 0 // no STOP at the end, a repeated START follows
#define I2C_STOP 1     // send STOP after the last byte

// Clears any error and leaves the peripheral idle for the next transfer
static void i2c_reset(void) {
  HAL_Delay(1); // let a STOP the hardware already started finish
  I2C->CR1 &= ~I2C_CR1_PE;
  HAL_Delay(1); // PE must stay low for a few clock cycles
  I2C->CR1 |= I2C_CR1_PE;
}

// Waits for a status flag. Gives up if the device NACKs or on timeout.
static int i2c_wait_for(uint32_t flag) {
  uint32_t start = HAL_GetTick();

  while (!(I2C->ISR & flag)) {
    if ((I2C->ISR & I2C_ISR_NACKF) ||
        (HAL_GetTick() - start) > I2C_TIMEOUT_MS) {
      i2c_reset();
      return -1;
    }
  }
  return 0;
}

// Sends START + address, then nbytes are transferred in the given direction
static void i2c_start(uint8_t addr, uint8_t nbytes, int direction, int stop) {
  I2C->CR2 = ((uint32_t)(addr << 1) & I2C_CR2_SADD) |
             ((uint32_t)nbytes << I2C_CR2_NBYTES_Pos) |
             (direction == I2C_READ ? I2C_CR2_RD_WRN : 0) |
             (stop == I2C_STOP ? I2C_CR2_AUTOEND : 0) | I2C_CR2_START;
}

static int i2c_send_byte(uint8_t byte) {
  if (i2c_wait_for(I2C_ISR_TXIS) != 0) {
    return -1;
  }
  I2C->TXDR = byte;
  return 0;
}

static int i2c_wait_for_stop(void) {
  if (i2c_wait_for(I2C_ISR_STOPF) != 0) {
    return -1;
  }
  I2C->ICR = I2C_ICR_STOPCF;
  return 0;
}

int i2c_probe(uint8_t addr) {
  uint32_t start = HAL_GetTick();
  int acked;

  // Zero data bytes: START, address, then STOP right away
  i2c_start(addr, 0, I2C_WRITE, I2C_STOP);

  // STOP is sent whether or not the address was ACKed
  while (!(I2C->ISR & I2C_ISR_STOPF)) {
    if ((HAL_GetTick() - start) > I2C_TIMEOUT_MS) {
      i2c_reset();
      return -1;
    }
  }
  acked = !(I2C->ISR & I2C_ISR_NACKF);
  I2C->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF;
  return acked ? 0 : -1;
}

int i2c_write_reg(uint8_t addr, uint8_t reg, const uint8_t *data,
                  uint8_t len) {
  i2c_start(addr, 1 + len, I2C_WRITE, I2C_STOP);
  if (i2c_send_byte(reg) != 0) {
    return -1;
  }
  for (uint8_t i = 0; i < len; i++) {
    if (i2c_send_byte(data[i]) != 0) {
      return -1;
    }
  }
  return i2c_wait_for_stop();
}

int i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
  // Send the register address without a STOP...
  i2c_start(addr, 1, I2C_WRITE, I2C_KEEP_BUS);
  if (i2c_send_byte(reg) != 0 || i2c_wait_for(I2C_ISR_TC) != 0) {
    return -1;
  }

  // ...then a repeated START to read the data back
  i2c_start(addr, len, I2C_READ, I2C_STOP);
  for (uint8_t i = 0; i < len; i++) {
    if (i2c_wait_for(I2C_ISR_RXNE) != 0) {
      return -1;
    }
    data[i] = (uint8_t)I2C->RXDR;
  }
  return i2c_wait_for_stop();
}
