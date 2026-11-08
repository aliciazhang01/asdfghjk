#ifndef MCP3561_H
#define MCP3561_H

#include <stdint.h>

// MCP3561R 24-bit delta-sigma ADC (U3) on SPI1. CH0 is FSR_SENSE. Register
// numbers and bit fields are from the MCP3561/2/4R datasheet (link in the
// top-level README).
//
// Every SPI frame starts with a command byte, and while that byte goes out
// the chip sends back its STATUS byte. Data bytes follow, MSB first.

#define MCP3561_DEVICE_ADDR 0x1 // factory default for the MCP3561R-E/ST
#define MCP3561_VREF_MV 2400    // internal reference

// Command types (bits 1:0 of the command byte)
#define MCP3561_FAST_COMMAND 0x0
#define MCP3561_STATIC_READ 0x1
#define MCP3561_INC_WRITE 0x2
#define MCP3561_INC_READ 0x3

// Builds a command byte: device address, register (or fast command), type
#define MCP3561_COMMAND(reg, type)                                             \
  ((MCP3561_DEVICE_ADDR << 6) | ((reg) << 2) | (type))

// Registers
#define MCP3561_REG_ADCDATA 0x0
#define MCP3561_REG_CONFIG0 0x1
#define MCP3561_REG_CONFIG1 0x2
#define MCP3561_REG_CONFIG2 0x3
#define MCP3561_REG_CONFIG3 0x4
#define MCP3561_REG_IRQ 0x5
#define MCP3561_REG_MUX 0x6
#define MCP3561_REG_CHIP_ID 0xE // "RESERVED" register, low nibble = part

// Fast commands (go where the register number would)
#define MCP3561_FAST_START 0xA // restart conversion
#define MCP3561_FAST_RESET 0xE // reset all registers to defaults

#define MCP3561_CHIP_ID 0xC // MCP3561R (0xD = MCP3562R, 0xF = MCP3564R)

// Oversampling ratio, CONFIG1 bits 5:2. Higher = slower and less noisy.
typedef enum {
  MCP3561_OSR_32 = 0x0,
  MCP3561_OSR_64 = 0x1,
  MCP3561_OSR_128 = 0x2,
  MCP3561_OSR_256 = 0x3,
  MCP3561_OSR_512 = 0x4,
  MCP3561_OSR_1024 = 0x5,
  MCP3561_OSR_2048 = 0x6,
  MCP3561_OSR_4096 = 0x7,
} mcp3561_osr_t;

typedef struct {
  int32_t code;       // raw 24-bit result, sign-extended
  float millivolts;   // code scaled by the reference
} mcp3561_reading_t;

// Sends a command byte followed by len data bytes (tx may be NULL to send
// zeros) and stores the bytes clocked back in rx (may be NULL). Returns 0 if
// the STATUS byte shows the chip is there.
int mcp3561_transfer(uint8_t command, const uint8_t *tx, uint8_t *rx,
                     uint8_t len, uint8_t *status);

// Resets the chip, checks its ID and configures it to convert CH0
// continuously. Returns 0 on success.
int mcp3561_init(mcp3561_osr_t osr);

// 1 when the ADC_IRQ pin says a new conversion is waiting
int mcp3561_data_ready(void);

// Reads the latest conversion. Returns 0 on success.
int mcp3561_read(mcp3561_reading_t *reading);

#endif
