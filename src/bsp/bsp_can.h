#pragma once

#include <stdint.h>

#include "bsp.h"
#include "hal_can.h"

#define CAN_DATA_SIZE (8)

#define CAN_MAX_SUBER_NUMBER (10)

typedef enum {
  BSP_CAN_1,
  BSP_CAN_2,
  BSP_CAN_NUM,
  BSP_CAN_ERR,
} bsp_can_t;

typedef enum {
  HAL_CAN_TX_MAILBOX0_CPLT_CB,
  HAL_CAN_TX_MAILBOX1_CPLT_CB,
  HAL_CAN_TX_MAILBOX2_CPLT_CB,
  HAL_CAN_TX_MAILBOX0_ABORT_CB,
  HAL_CAN_TX_MAILBOX1_ABORT_CB,
  HAL_CAN_TX_MAILBOX2_ABORT_CB,
  HAL_CAN_RX_FIFO0_MSG_PENDING_CB,
  HAL_CAN_RX_FIFO0_FULL_CB,
  HAL_CAN_RX_FIFO1_MSG_PENDING_CB,
  HAL_CAN_RX_FIFO1_FULL_CB,
  HAL_CAN_SLEEP_CB,
  HAL_CAN_WAKEUP_FROM_RX_MSG_CB,
  HAL_CAN_ERROR_CB,
  BSP_CAN_CB_NUM
} bsp_can_callback_t;

typedef struct {
  CAN_RxHeaderTypeDef header;
  uint8_t data[CAN_DATA_SIZE];
} can_rawrx_t;

typedef struct {
  CAN_TxHeaderTypeDef header;
  uint8_t data[CAN_DATA_SIZE];
} can_rawtx_t;

typedef struct {
  uint32_t can_id;
  uint8_t data[CAN_DATA_SIZE];
} can_tx_item_t;

typedef struct {
  uint32_t index;
  uint8_t data[CAN_DATA_SIZE];
} can_rx_item_t;

typedef struct {
  uint32_t index;
  uint32_t number;
  void (*cb)(can_rx_item_t *, void *);
  void *callback_arg;
} can_suber_t;

typedef struct {
  can_suber_t suber[CAN_MAX_SUBER_NUMBER];
  uint8_t suber_number;
} can_group_t;

CAN_HandleTypeDef *bsp_can_get_handle(bsp_can_t can);
int8_t bsp_can_register_callback(bsp_can_t can, bsp_can_callback_t type,
                                void (*callback)(void *), void *callback_arg);
int8_t bsp_can_publish_data(bsp_can_t can, uint32_t StdId, uint8_t *data);
int8_t bsp_can_register_subscriber(bsp_can_t can, uint32_t index,
                                  uint32_t number,
                                  void (*cb)(can_rx_item_t *, void *),
                                  void *callback_arg);
int8_t can_trans_packet(bsp_can_t can, uint32_t StdId, uint8_t *data,
                        uint32_t *mailbox);
