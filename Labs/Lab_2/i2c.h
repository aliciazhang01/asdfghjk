#ifndef LAB_I2C_H
#define LAB_I2C_H

#include <stdint.h>

// Blocking I2C master on I2C1 (PB6 = SCL, PB7 = SDA). This file is given to
// you, you don't need to change it. See i2c.c for why it talks to the I2C
// registers directly instead of using the HAL.
//
// Addresses are 7-bit (e.g. 0x68), the read/write bit is added for you.
// Every function returns 0 on success and -1 if the device didn't answer
// (NACK) or the bus timed out.

// Sends just the address and checks if any device ACKs it
int i2c_probe(uint8_t addr);

// START, addr+W, reg, data[0..len-1], STOP
int i2c_write_reg(uint8_t addr, uint8_t reg, const uint8_t *data, uint8_t len);

// START, addr+W, reg, repeated START, addr+R, data[0..len-1], STOP
int i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);

#endif
