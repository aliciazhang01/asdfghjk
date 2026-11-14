#include "lab_4.h"
#include "main.h"

#define CAN_BITRATE 500000U
#define SEND_PERIOD_MS 100U
#define TSEG1 13U
#define TSEG2 2U

extern FDCAN_HandleTypeDef hfdcan1;
lab_4_bus_t lab_4_bus;
volatile uint32_t lab_4_send_failures;
volatile uint32_t lab_4_hal_error;
volatile uint32_t lab_4_setup_stage;

int can_setup(uint32_t bitrate, uint32_t mode, uint32_t std_filters) {
  uint32_t clock = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_FDCAN1);
  uint32_t quanta = 1U + TSEG1 + TSEG2;
  if (bitrate == 0U || clock == 0U || clock % bitrate != 0U ||
      (clock / bitrate) % quanta != 0U) {
    return -1;
  }
  uint32_t prescaler = clock / bitrate / quanta;
  if (prescaler < 1U || prescaler > 512U) {
    return -1;
  }
  hfdcan1.Init.ClockDivider = FDCAN_CLOCK_DIV1;
  hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  hfdcan1.Init.Mode = mode;
  // A failed frame is replaced by the next periodic message, not retried forever.
  hfdcan1.Init.AutoRetransmission = DISABLE;
  hfdcan1.Init.TransmitPause = DISABLE;
  hfdcan1.Init.ProtocolException = DISABLE;
  hfdcan1.Init.NominalPrescaler = prescaler;
  hfdcan1.Init.NominalTimeSeg1 = TSEG1;
  hfdcan1.Init.NominalTimeSeg2 = TSEG2;
  hfdcan1.Init.NominalSyncJumpWidth = TSEG2;
  hfdcan1.Init.StdFiltersNbr = std_filters;
  hfdcan1.Init.ExtFiltersNbr = 0;
  hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
  return HAL_FDCAN_Init(&hfdcan1) == HAL_OK ? 0 : -1;
}

static void check_bus(void) {
  FDCAN_ErrorCountersTypeDef counters = {0};
  FDCAN_ProtocolStatusTypeDef status = {0};
  if (HAL_FDCAN_GetErrorCounters(&hfdcan1, &counters) == HAL_OK) {
    lab_4_bus.tx_errors = counters.TxErrorCnt;
    lab_4_bus.rx_errors = counters.RxErrorCnt;
  }
  if (HAL_FDCAN_GetProtocolStatus(&hfdcan1, &status) == HAL_OK) {
    lab_4_bus.error_passive = status.ErrorPassive;
    lab_4_bus.bus_off = status.BusOff;
    if (status.BusOff) {
      CLEAR_BIT(hfdcan1.Instance->CCCR, FDCAN_CCCR_INIT);
    }
  }
  lab_4_hal_error = HAL_FDCAN_GetError(&hfdcan1);
}

void lab_4(void) {
  FDCAN_TxHeaderTypeDef header = {0};
  uint32_t counter = 0;
  lab_4_setup_stage = 1;
  if (can_setup(CAN_BITRATE, FDCAN_MODE_NORMAL, 0) != 0) {
    lab_4_hal_error = HAL_FDCAN_GetError(&hfdcan1);
    Error_Handler();
    return;
  }
  lab_4_setup_stage = 2;
  if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT,
                                  FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE) != HAL_OK ||
      HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
    lab_4_hal_error = HAL_FDCAN_GetError(&hfdcan1);
    Error_Handler();
    return;
  }
  lab_4_setup_stage = 3;
  header.Identifier = LAB_4_HEARTBEAT_ID;
  header.IdType = FDCAN_STANDARD_ID;
  header.TxFrameType = FDCAN_DATA_FRAME;
  header.DataLength = FDCAN_DLC_BYTES_8;
  header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  header.BitRateSwitch = FDCAN_BRS_OFF;
  header.FDFormat = FDCAN_CLASSIC_CAN;
  header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;

  uint32_t last_send = HAL_GetTick() - SEND_PERIOD_MS;
  while (1) {
    uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - last_send) >= SEND_PERIOD_MS) {
      last_send = now;
      // Two little-endian uint32 values: sequence number and uptime in ms.
      uint8_t data[8];
      for (uint32_t i = 0; i < 4; ++i) {
        data[i] = (uint8_t)(counter >> (8U * i));
        data[i + 4] = (uint8_t)(now >> (8U * i));
      }
      if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &header, data) == HAL_OK) {
        ++lab_4_bus.sent;
        ++counter;
        HAL_GPIO_TogglePin(Status_LED_GPIO_Port, Status_LED_Pin);
      } else {
        ++lab_4_send_failures;
      }
      check_bus();
    }
    HAL_Delay(1);
  }
}
