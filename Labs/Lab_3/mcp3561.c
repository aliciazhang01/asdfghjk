#include "mcp3561.h"
#include "main.h"
#include <string.h>

extern SPI_HandleTypeDef hspi1;

#define MAX_DATA 6 // longest transfer we do: CONFIG0..MUX
#define FULL_SCALE (1 << 23) // a code of 2^23 means VIN = VREF

// STATUS byte: 0 0 A1 A0 !A0 DR CRC POR. Bits 5:3 echo the device address
// (with A0 repeated inverted).
#define STATUS_ADDR(status) (((status) >> 3) & 0x7)
#define STATUS_ADDR_EXPECTED                                                   \
  ((MCP3561_DEVICE_ADDR << 1) | (~MCP3561_DEVICE_ADDR & 1))

// IRQ bits 6:4 are read-only status flags, so skip them when verifying
static const uint8_t config_check_mask[MAX_DATA] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF,
};

int mcp3561_transfer(uint8_t command, const uint8_t *tx, uint8_t *rx,
                     uint8_t len, uint8_t *status) {
  uint8_t tx_buf[1 + MAX_DATA] = {command};
  uint8_t rx_buf[1 + MAX_DATA] = {0};

  if (len > MAX_DATA) {
    return -1;
  }

  // Data bytes go after the command byte (zeros if tx is NULL)
  if (tx != NULL) {
    memcpy(&tx_buf[1], tx, len);
  }

  // NSS (PA4) is driven by the SPI peripheral for the whole transfer
  if (HAL_SPI_TransmitReceive(&hspi1, tx_buf, rx_buf, 1 + len, 10) != HAL_OK) {
    return -1;
  }

  // STATUS comes back while the command byte goes out
  if (status != NULL) {
    *status = rx_buf[0];
  }
  if (rx != NULL) {
    memcpy(rx, &rx_buf[1], len);
  }

  return (STATUS_ADDR(rx_buf[0]) == STATUS_ADDR_EXPECTED) ? 0 : -1;
}

static int mcp3561_read_regs(uint8_t reg, uint8_t *data, uint8_t len) {
  return mcp3561_transfer(MCP3561_COMMAND(reg, MCP3561_INC_READ), NULL, data,
                          len, NULL);
}

static int mcp3561_write_regs(uint8_t reg, const uint8_t *data, uint8_t len) {
  return mcp3561_transfer(MCP3561_COMMAND(reg, MCP3561_INC_WRITE), data, NULL,
                          len, NULL);
}

int mcp3561_init(mcp3561_osr_t osr) {
  uint8_t id[2] = {0};
  uint8_t readback[MAX_DATA];

  // CONFIG0..MUX, written in one go.
  const uint8_t config[MAX_DATA] = {
      0b11100011, // CONFIG0: internal VREF, not shutdown, internal clock (no output), no CS bias, conversion mode
      (uint8_t)(osr << 2), // CONFIG1: prescaler /1, OSR in bits 5:2
      0x8B,       // CONFIG2: boost 1x, gain 1x, auto-zero off (defaults, given)
      0b11000000, // CONFIG3: continuous conversions, 24-bit data, CRC off, cal off
      0b00001010, // IRQ: IRQ pin, push-pull idle high, fast commands on, start interrupt off
      0b00001000, // MUX: VIN+ = CH0, VIN- = AGND
  };

  if (mcp3561_transfer(MCP3561_COMMAND(MCP3561_FAST_RESET, MCP3561_FAST_COMMAND),
                       NULL, NULL, 0, NULL) != 0) {
    return -1;
  }
  HAL_Delay(1);

  if (mcp3561_read_regs(MCP3561_REG_CHIP_ID, id, sizeof(id)) != 0 ||
      (id[1] & 0x0F) != MCP3561_CHIP_ID) {
    return -1;
  }

  // Write the config, then read it back to make sure it stuck
  if (mcp3561_write_regs(MCP3561_REG_CONFIG0, config, sizeof(config)) != 0 ||
      mcp3561_read_regs(MCP3561_REG_CONFIG0, readback, sizeof(readback)) != 0) {
    return -1;
  }
  for (int i = 0; i < MAX_DATA; i++) {
    if ((readback[i] & config_check_mask[i]) !=
        (config[i] & config_check_mask[i])) {
      return -1;
    }
  }
  return 0;
}

int mcp3561_data_ready(void) {
  // ADC_IRQ (PB4) is active low: low means a new result is ready
  return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == GPIO_PIN_RESET;
}

int mcp3561_read(mcp3561_reading_t *reading) {
  uint8_t data[3];

  if (mcp3561_read_regs(MCP3561_REG_ADCDATA, data, sizeof(data)) != 0) {
    return -1;
  }

  // 24-bit two's complement, MSB first
  int32_t code = ((int32_t)data[0] << 16) | ((int32_t)data[1] << 8) | data[2];
  if (code & 0x800000) {
    code |= 0xFF000000; // negative: sign-extend from bit 23
  }

  reading->code = code;
  reading->millivolts = (float)code * MCP3561_VREF_MV / FULL_SCALE;
  return 0;
}