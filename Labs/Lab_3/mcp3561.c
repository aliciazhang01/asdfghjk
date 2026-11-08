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

  // TODO (part 1):
  //  1. copy tx (if it isn't NULL) into tx_buf after the command byte
  //  2. send tx_buf and receive into rx_buf at the same time with
  //     HAL_SPI_TransmitReceive (timeout 10 ms). NSS (PA4) is driven by the
  //     SPI peripheral, low for the whole transfer, so you don't touch it.
  //  3. save the STATUS byte into *status and the data bytes into rx (skip
  //     either one if it's NULL)
  //  4. return 0 if the address bits in STATUS match, -1 otherwise
  (void)tx_buf;
  (void)rx_buf;
  return -1;
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
  // TODO (part 2): work out each byte from the register map in the datasheet.
  // See the table in the handout for the settings you need.
  const uint8_t config[MAX_DATA] = {
      0x00, // CONFIG0: internal VREF, internal clock, conversion mode
      0x00, // CONFIG1: no prescaler, oversampling ratio = osr
      0x8B, // CONFIG2: boost 1x, gain 1x, auto-zero off (defaults, given)
      0x00, // CONFIG3: continuous conversions, 24-bit data
      0x00, // IRQ: IRQ pin push-pull idle high, fast commands on
      0x00, // MUX: VIN+ = CH0, VIN- = AGND
  };
  (void)osr;

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
  // TODO (part 3): read the ADC_IRQ pin. It goes low when a new result is
  // ready.
  return 0;
}

int mcp3561_read(mcp3561_reading_t *reading) {
  uint8_t data[3];

  if (mcp3561_read_regs(MCP3561_REG_ADCDATA, data, sizeof(data)) != 0) {
    return -1;
  }

  // TODO (part 3): data[0..2] is a 24-bit two's complement number, MSB
  // first. Put it together into reading->code (an int32_t, so negative
  // values have to be sign-extended by hand), then convert to millivolts. A
  // code of FULL_SCALE means the input equals the reference voltage.
  (void)reading;
  return 0;
}
