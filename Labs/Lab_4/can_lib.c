#include "can_lib.h"
#include <stdlib.h>
#include <string.h>

void add_cantx_message(tx_messages_t *tx_msgs, message_header_t msg) {
  tx_msgs->messages[tx_msgs->message_count].name = strdup(msg.name);
  tx_msgs->messages[tx_msgs->message_count].header = msg.header;
  tx_msgs->messages[tx_msgs->message_count].handler.construct_message =
      msg.handler.construct_message;
  tx_msgs->messages[tx_msgs->message_count].speed = msg.speed;

  tx_msgs->message_count++;
}

void add_canrx_message(rx_messages_t *rx_msgs, message_header_t msg) {
  rx_msgs->messages[rx_msgs->message_count].name = strdup(msg.name);
  rx_msgs->messages[rx_msgs->message_count].header = msg.header;
  rx_msgs->messages[rx_msgs->message_count].handler.parse_message =
      msg.handler.parse_message;

  rx_msgs->message_count++;
}

int configure_filter(rx_messages_t *rx_msgs,
                     FDCAN_HandleTypeDef *hfdcan) { // add bank offset later

  if (rx_msgs->message_count > 2 * hfdcan->Init.StdFiltersNbr) {
    printf("Exceed Max allocation, increase StdFiltersNbr or use a mask\n");
    return HAL_ERROR;
  }

  FDCAN_FilterTypeDef canfilterconfig;

  for (uint8_t msg_idx = 0, filter_index = 0; msg_idx < rx_msgs->message_count;
       msg_idx += 2, filter_index++) {
    canfilterconfig.IdType = FDCAN_STANDARD_ID;
    canfilterconfig.FilterIndex = filter_index;
    canfilterconfig.FilterType = FDCAN_FILTER_DUAL;
    canfilterconfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;

    uint32_t id1 = rx_msgs->messages[msg_idx].header.RxHeader_.Identifier;
    uint32_t id2 =
        (msg_idx + 1 < rx_msgs->message_count)
            ? rx_msgs->messages[msg_idx + 1].header.RxHeader_.Identifier
            : id1;
    canfilterconfig.FilterID1 = id1;
    canfilterconfig.FilterID2 = id2;

    if (HAL_FDCAN_ConfigFilter(hfdcan, &canfilterconfig) != HAL_OK) {
      return HAL_ERROR;
    }
  }
  if (HAL_FDCAN_ConfigGlobalFilter(hfdcan, FDCAN_REJECT, FDCAN_REJECT,
                                   FDCAN_REJECT_REMOTE,
                                   FDCAN_REJECT_REMOTE) != HAL_OK) {
    return HAL_ERROR;
  }
  return HAL_OK;
}

void parse_canrx_message(rx_messages_t *rx_msgs,
                         FDCAN_RxHeaderTypeDef *RxHeader, uint8_t *rxData) {
  for (uint8_t i = 0; i < rx_msgs->message_count; i++) {
    if (rx_msgs->messages[i].handler.parse_message != NULL &&
        rx_msgs->messages[i].header.RxHeader_.Identifier ==
            RxHeader->Identifier) {
      rx_msgs->messages[i].handler.parse_message(rxData);
      return;
    }
  }
}

void send_cantx_messages(FDCAN_HandleTypeDef *hfdcan, tx_messages_t *tx_msgs,
                         can_speed_t speed) {
  uint32_t TxBufferIndex = 0;
  for (uint8_t i = 0; i < tx_msgs->message_count; i++) {
    if (tx_msgs->messages[i].handler.construct_message != NULL &&
        tx_msgs->messages[i].speed == speed) {
      for (uint8_t c = 0; c < 8; c++) {
        tx_msgs->messages[i].TxData_[c] = 0;
      }
      tx_msgs->messages[i].handler.construct_message(&tx_msgs->messages[i]);
      if (send_can_message(hfdcan, &tx_msgs->messages[i].header.TxHeader_,
                           tx_msgs->messages[i].TxData_, &TxBufferIndex) != 0) {
        continue;
      }

      uint32_t start = HAL_GetTick();
      while (HAL_FDCAN_IsTxBufferMessagePending(hfdcan, TxBufferIndex)) {
        if ((HAL_GetTick() - start) > 5U) {
          break;
        }
      }
    }
  }
}

uint8_t send_can_message(FDCAN_HandleTypeDef *hfdcan,
                         FDCAN_TxHeaderTypeDef *TxHeader, uint8_t *data,
                         uint32_t *TxBufferIndex) {
  if (HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, TxHeader, data) != HAL_OK) {
    return 1;
  }
  *TxBufferIndex = HAL_FDCAN_GetLatestTxFifoQRequestBuffer(hfdcan);
  return 0;
}
