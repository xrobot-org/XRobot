#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"

typedef enum {
  BSP_CAN_1,
  BSP_CAN_2,
  BSP_CAN_3,
  BSP_CAN_4,
  BSP_CAN_NUM,
  BSP_CAN_ERR,
} bsp_can_t;

typedef enum {
  CAN_RX_MSG_CALLBACK,
  CAN_TX_CPLT_CALLBACK,
  CANFD_RX_MSG_CALLBACK,
  CANFD_TX_CPLT_CALLBACK,
  BSP_CAN_CB_NUM
} bsp_can_callback_t;

typedef enum {
  CAN_FORMAT_STD,
  CAN_FORMAT_EXT,
} bsp_can_format_t;

typedef struct {
  uint8_t data[8];
} bsp_can_data_t;

typedef struct {
  size_t size;
  uint8_t *data;
} bsp_canfd_data_t;

void bsp_can_init(void);
bsp_status_t bsp_can_register_callback(
    bsp_can_t can, bsp_can_callback_t type,
    void (*callback)(bsp_can_t can, uint32_t id, uint8_t *data, void *arg),
    void *callback_arg);
bsp_status_t bsp_can_trans_packet(bsp_can_t can, bsp_can_format_t format,
                                  uint32_t id, uint8_t *data);

bsp_status_t bsp_canfd_trans_packet(bsp_can_t can, bsp_can_format_t format,
                                    uint32_t id, uint8_t *data, size_t size);

bsp_status_t bsp_can_get_msg(bsp_can_t can, uint8_t *data, uint32_t *index);

#ifdef __cplusplus
}
#endif
