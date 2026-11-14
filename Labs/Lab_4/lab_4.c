#include "lab_4.h"
#include "bmi323.h"
#include "can_lib.h"
#include "main.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Which part of the lab runs. Change this, rebuild, and flash.
#define LAB_4_PART 4

#define BITRATE 500000

// FDCAN1 kernel clock is HSE (8 MHz). Each bit is split into time quanta:
// 1 sync + TSEG1 + TSEG2. The sample point is at the end of TSEG1.
#define FDCAN_CLOCK_HZ 8000000
#define TSEG1 13
#define TSEG2 2
#define TQ_PER_BIT (1 + TSEG1 + TSEG2)

#define TELEMETRY_PERIOD_MS 100


extern FDCAN_HandleTypeDef hfdcan1;

lab_4_bus_t lab_4_bus;

// LED state set by the LED command message
typedef enum { LED_OFF = 0, LED_ON = 1, LED_BLINK = 2 } led_mode_t;
static led_mode_t led_mode = LED_BLINK;
static uint16_t led_period_ms = 1000;

static bmi323_reading_t imu;
static int16_t partner_acc_z_mg;

// ---------------------------------------------------------------- helpers

int can_setup(uint32_t bitrate, uint32_t mode, uint32_t std_filters) {
  // Bitrate = FDCAN_CLOCK_HZ / (prescaler * TQ_PER_BIT)
  // CubeMX left prescaler 16: 8,000,000 / (16 * 16) = 31,250 bit/s.
  if ((FDCAN_CLOCK_HZ % (bitrate * TQ_PER_BIT)) != 0) {
    return -1;
  }
  uint32_t prescaler = FDCAN_CLOCK_HZ / (bitrate * TQ_PER_BIT);

  hfdcan1.Init.NominalPrescaler = prescaler;
  hfdcan1.Init.NominalTimeSeg1 = TSEG1;
  hfdcan1.Init.NominalTimeSeg2 = TSEG2;
  hfdcan1.Init.NominalSyncJumpWidth = TSEG2;
  hfdcan1.Init.Mode = mode;
  hfdcan1.Init.StdFiltersNbr = std_filters;

  return HAL_FDCAN_Init(&hfdcan1) == HAL_OK ? 0 : -1;
}

static void halt(const char *message) {
  printf("CAN: %s\r\n", message);
  while (1) {
  }
}

// Updates lab_4_bus from the error counters and rejoins the bus after bus-off
static void check_bus(void) {
  FDCAN_ErrorCountersTypeDef counters;
  FDCAN_ProtocolStatusTypeDef status;

  HAL_FDCAN_GetErrorCounters(&hfdcan1, &counters);
  HAL_FDCAN_GetProtocolStatus(&hfdcan1, &status);
  lab_4_bus.tx_errors = counters.TxErrorCnt;
  lab_4_bus.rx_errors = counters.RxErrorCnt;
  lab_4_bus.error_passive = status.ErrorPassive;
  lab_4_bus.bus_off = status.BusOff;

  if (status.BusOff) {
    // bus-off sets INIT; clearing it rejoins after 129x11 recessive bits
    CLEAR_BIT(hfdcan1.Instance->CCCR, FDCAN_CCCR_INIT);
  }
}

static FDCAN_TxHeaderTypeDef tx_header(uint32_t id, uint32_t dlc) {
  FDCAN_TxHeaderTypeDef header = {0};

  header.Identifier = id;
  header.IdType = FDCAN_STANDARD_ID;
  header.TxFrameType = FDCAN_DATA_FRAME;
  header.DataLength = dlc;
  header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  header.BitRateSwitch = FDCAN_BRS_OFF;
  header.FDFormat = FDCAN_CLASSIC_CAN;
  header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  return header;
}

static void print_frame(const char *dir, uint32_t id, uint32_t dlc,
                        const uint8_t *data) {
  printf("CAN: %s 0x%03lx [%lu]", dir, (unsigned long)id, (unsigned long)dlc);
  for (uint32_t i = 0; i < dlc && i < 8; i++) {
    printf(" %02x", data[i]);
  }
  printf("\r\n");
}

static void put_le16(uint8_t *dst, int16_t v) {
  dst[0] = (uint8_t)v;
  dst[1] = (uint8_t)((uint16_t)v >> 8);
}

static int16_t get_le16(const uint8_t *src) {
  return (int16_t)(src[0] | (src[1] << 8));
}

// ---------------------------------------------------------------- part 2, 3

// Sends a heartbeat (counter + tick) every 100 ms and prints whatever comes
// back. In loopback, also checks that each frame comes back unchanged.
static void heartbeat(int loopback) {
  FDCAN_TxHeaderTypeDef header = tx_header(LAB_4_HEARTBEAT_ID, FDCAN_DLC_BYTES_8);
  FDCAN_RxHeaderTypeDef rx_header;
  uint8_t tx_data[8] = {0}, rx_data[8];
  uint8_t counter = 0;

  while (1) {
    uint32_t tick = HAL_GetTick();

    tx_data[0] = counter;
    counter++;
    tx_data[4] = (uint8_t)tick;
    tx_data[5] = (uint8_t)(tick >> 8);
    tx_data[6] = (uint8_t)(tick >> 16);
    tx_data[7] = (uint8_t)(tick >> 24);

    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &header, tx_data) == HAL_OK) {
      lab_4_bus.sent++;
    }

    HAL_Delay(100);

    while (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) > 0 &&
           HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &rx_header,
                                  rx_data) == HAL_OK) {
      lab_4_bus.received++;
      print_frame("rx", rx_header.Identifier, rx_header.DataLength, rx_data);
      if (loopback && memcmp(rx_data, tx_data, sizeof(tx_data)) != 0) {
        lab_4_bus.mismatched++;
      }
    }

    check_bus();
  }
}

// ---------------------------------------------------------------- part 4, 5

// [0..1] FSR mV, [2..7] accel X/Y/Z in mg, all int16 little endian
static void construct_telemetry(message_header_t *msg) {
  put_le16(&msg->TxData_[0], 0);   // force sensor not used
  for (int i = 0; i < 3; i++) {
    put_le16(&msg->TxData_[2 + 2 * i], (int16_t)(imu.acc_g[i] * 1000.0f));
  }
}

// [0] mode (0 off, 1 on, 2 blink), [1..2] blink period in ms (uint16 LE)
static void parse_led_command(uint8_t *data) {
  if (data[0] <= LED_BLINK) {
    led_mode = (led_mode_t)data[0];
  }

  // A period of 0 would make "now - last_toggle >= period / 2" always true,
  // so the LED would toggle every loop pass. Ignore it.
  uint16_t period = (uint16_t)(data[1] | (data[2] << 8));
  if (period != 0) {
    led_period_ms = period;
  }
}

// Partner's telemetry: only the Z acceleration is used (bytes 6..7)
static void parse_partner_telemetry(uint8_t *data) {
  partner_acc_z_mg = get_le16(&data[6]);
}

static void update_led(void) {
  static uint32_t last_toggle = 0;
  uint32_t now = HAL_GetTick();

  if (LAB_4_PART == 5) {
    // Light up while the partner's board is upside down
    HAL_GPIO_WritePin(Status_LED_GPIO_Port, Status_LED_Pin,
                      partner_acc_z_mg < -500 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return;
  }

  switch (led_mode) {
  case LED_OFF:
    HAL_GPIO_WritePin(Status_LED_GPIO_Port, Status_LED_Pin, GPIO_PIN_RESET);
    break;
  case LED_ON:
    HAL_GPIO_WritePin(Status_LED_GPIO_Port, Status_LED_Pin, GPIO_PIN_SET);
    break;
  case LED_BLINK:
    if (now - last_toggle >= led_period_ms / 2) {
      last_toggle = now;
      HAL_GPIO_TogglePin(Status_LED_GPIO_Port, Status_LED_Pin);
    }
    break;
  }
}

static void telemetry(void) {
  static tx_messages_t tx_msgs = {0};
  static rx_messages_t rx_msgs = {0};
  message_header_t msg;
  FDCAN_RxHeaderTypeDef rx_header;
  uint8_t rx_data[8];
  uint32_t last_send = 0;

  if (bmi323_init() != 0) {
    halt("IMU init failed, check lab 2");
  }

  // What we send
  memset(&msg, 0, sizeof(msg));
  msg.name = "telemetry";
  msg.header.TxHeader_ = tx_header(LAB_4_TELEMETRY_BASE + NODE_ID, FDCAN_DLC_BYTES_8);
  msg.handler.construct_message = construct_telemetry;
  msg.speed = FAST;
  add_cantx_message(&tx_msgs, msg);

  // What we listen for. Everything else is filtered out in hardware.
  memset(&msg, 0, sizeof(msg));
  msg.name = "led_cmd";
  msg.header.RxHeader_.Identifier = LAB_4_LED_CMD_BASE + NODE_ID;
  msg.handler.parse_message = parse_led_command;
  add_canrx_message(&rx_msgs, msg);

  if (LAB_4_PART == 5) {
    memset(&msg, 0, sizeof(msg));
    msg.name = "partner";
    msg.header.RxHeader_.Identifier = LAB_4_TELEMETRY_BASE + PARTNER_ID;
    msg.handler.parse_message = parse_partner_telemetry;
    add_canrx_message(&rx_msgs, msg);
  }

  if (configure_filter(&rx_msgs, &hfdcan1) != HAL_OK) {
    halt("filter setup failed");
  }
  if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
    halt("start failed");
  }

  while (1) {
    if (HAL_GetTick() - last_send >= TELEMETRY_PERIOD_MS) {
      last_send = HAL_GetTick();
      bmi323_read(&imu);
      send_cantx_messages(&hfdcan1, &tx_msgs, FAST);
      lab_4_bus.sent++;
      check_bus();
    }

    while (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) > 0 &&
           HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &rx_header,
                                  rx_data) == HAL_OK) {
      lab_4_bus.received++;
      parse_canrx_message(&rx_msgs, &rx_header, rx_data);
    }

    update_led();
  }
}

// ---------------------------------------------------------------- main

void lab_4(void) {
  printf("lab 4 part %d\r\n", LAB_4_PART);

  switch (LAB_4_PART) {
  case 1: // bit timing only, check hfdcan1.Init in the Watch panel
    if (can_setup(BITRATE, FDCAN_MODE_NORMAL, 0) != 0) {
      halt("setup failed");
    }
    printf("CAN: prescaler %lu\r\n", (unsigned long)hfdcan1.Init.NominalPrescaler);
    while (1) {
    }

  case 2: // talk to yourself, no wiring needed
    if (can_setup(BITRATE, FDCAN_MODE_INTERNAL_LOOPBACK, 0) != 0 ||
        HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
      halt("setup failed");
    }
    heartbeat(1);

  case 3: // a real bus
    if (can_setup(BITRATE, FDCAN_MODE_NORMAL, 0) != 0 ||
        HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
      halt("setup failed");
    }
    heartbeat(0);

  case 4: // telemetry out, LED commands in
  case 5: // react to your partner
    if (can_setup(BITRATE, FDCAN_MODE_NORMAL, 2) != 0) {
      halt("setup failed");
    }
    telemetry();
  }
}
</document_content>