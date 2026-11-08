#ifndef INC_CAN_LIB_H_
#define INC_CAN_LIB_H_

#define MESSAGE_MAX 20

#include "main.h"
#include <stdio.h>
#include <string.h>

typedef struct message_header message_header_t;
typedef struct tx_messages tx_messages_t;
typedef struct rx_messages rx_messages_t;
typedef enum can_speed { FAST = 0, SLOW } can_speed_t;

struct message_header {
  char *name;
  union {
    FDCAN_RxHeaderTypeDef RxHeader_;
    FDCAN_TxHeaderTypeDef TxHeader_;
  } header;
  uint8_t TxData_[8];
  union {
    void (*parse_message)(uint8_t *rxData); // save to global variables
    void (*construct_message)(
        message_header_t *msg); // construct data from global variables
  } handler;
  can_speed_t speed;
};

struct tx_messages {
  uint8_t message_count;
  message_header_t messages[MESSAGE_MAX];
};

struct rx_messages {
  uint8_t message_count;
  message_header_t messages[MESSAGE_MAX];
};

// initial population functions
void add_cantx_message(tx_messages_t *tx_msgs, message_header_t msg);
void add_canrx_message(rx_messages_t *rx_msgs, message_header_t msg);
int configure_filter(rx_messages_t *rx_msgs, FDCAN_HandleTypeDef *hfdcan);

void parse_canrx_message(rx_messages_t *rx_msgs, FDCAN_RxHeaderTypeDef *RxHeader,
                         uint8_t *rxData);

void send_cantx_messages(FDCAN_HandleTypeDef *hfdcan, tx_messages_t *tx_msgs,
                         can_speed_t speed);
uint8_t send_can_message(FDCAN_HandleTypeDef *hfdcan, FDCAN_TxHeaderTypeDef *TxHeader,
                         uint8_t *data, uint32_t *TxBufferIndex);

#endif /* INC_CAN_LIB_H_ */
