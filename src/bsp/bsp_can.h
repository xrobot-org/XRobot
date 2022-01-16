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
} BSP_CAN_t;

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
} BSP_CAN_Callback_t;

typedef struct {
  CAN_RxHeaderTypeDef header;
  uint8_t data[CAN_DATA_SIZE];
} CAN_RawRx_t;

typedef struct {
  CAN_TxHeaderTypeDef header;
  uint8_t data[CAN_DATA_SIZE];
} CAN_RawTx_t;

typedef struct {
  uint32_t StdId;
  uint8_t *data;
} CAN_Raw_t;

typedef struct {
  uint32_t index;
  uint32_t number;
  void (*cb)(CAN_Raw_t, void *);
  void *callback_arg;
} CAN_Suber_t;

typedef struct {
  CAN_Suber_t suber[CAN_MAX_SUBER_NUMBER];
  uint8_t suber_number;
} CAN_Group_t;

CAN_HandleTypeDef *BSP_CAN_GetHandle(BSP_CAN_t can);
int8_t BSP_CAN_RegisterCallback(BSP_CAN_t can, BSP_CAN_Callback_t type,
                                void (*callback)(void *), void *callback_arg);
int8_t BSP_CAN_PublishData(BSP_CAN_t can, CAN_RawRx_t *raw);
int8_t BSP_CAN_RegisterSubscriber(BSP_CAN_t can, uint32_t index,
                                  uint32_t number,
                                  void (*cb)(CAN_Raw_t, void *),
                                  void *callback_arg);
int8_t can_trans_packet(BSP_CAN_t can, uint32_t StdId, uint8_t *data,
                        uint32_t *mailbox);
