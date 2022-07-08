#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"
#include "om.h"

#define CAN_DATA_SIZE (8)

#define CAN_MAX_SUBER_NUMBER (10)

typedef enum {
  BSP_CAN_1,
  BSP_CAN_2,
  BSP_CAN_NUM,
  BSP_CAN_ERR,
} bsp_can_t;

// TODO：与HAL库隔离，去除不必要的类型
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
  uint32_t can_id;
  uint8_t data[CAN_DATA_SIZE];
} can_tx_item_t;

typedef struct {
  uint32_t index;
  uint8_t data[CAN_DATA_SIZE];
} can_rx_item_t;

void bsp_can_init(void);
om_topic_t *bsp_can_get_topic(bsp_can_t can);
int8_t bsp_can_register_callback(bsp_can_t can, bsp_can_callback_t type,
                                 void (*callback)(void *), void *callback_arg);
int8_t bsp_can_trans_packet(bsp_can_t can, uint32_t StdId, uint8_t *data,
                            uint32_t *mailbox, uint32_t timeout);
int8_t bsp_can_get_msg(bsp_can_t can, can_rx_item_t *item);
int8_t bsp_can_register_subscriber(bsp_can_t can, om_topic_t *sub,
                                   uint32_t index_id, uint32_t number);

#ifdef __cplusplus
}
#endif
