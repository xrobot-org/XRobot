#include "dev_can.h"

#include <stdbool.h>
#include <string.h>

#include "comp_utils.h"

static can_rawrx_t raw_rx1, raw_rx2;

static bool inited = false;

static void can_can1_rx_fifo_msg_pending_callback(void* arg) {
  UNUSED(arg);

  HAL_CAN_GetRxMessage(bsp_can_get_handle(BSP_CAN_1), CAN_FILTER_FIFO0,
                       &raw_rx1.header, raw_rx1.data);
  bsp_can_publish_data(BSP_CAN_1, raw_rx1.header.StdId, raw_rx1.data);
}

static void can_can2_rx_fifo_msg_pending_callback(void* arg) {
  UNUSED(arg);

  HAL_CAN_GetRxMessage(bsp_can_get_handle(BSP_CAN_2), CAN_FILTER_FIFO1,
                       &raw_rx2.header, raw_rx2.data);
  bsp_can_publish_data(BSP_CAN_2, raw_rx2.header.StdId, raw_rx2.data);
}

/* Exported functions ------------------------------------------------------- */
int8_t can_init(void) {
  if (inited) return DEVICE_ERR_INITED;

  CAN_FilterTypeDef can_filter = {0};

  can_filter.FilterBank = 0;
  can_filter.FilterIdHigh = 0;
  can_filter.FilterIdLow = 0;
  can_filter.FilterMode = CAN_FILTERMODE_IDMASK;
  can_filter.FilterScale = CAN_FILTERSCALE_32BIT;
  can_filter.FilterMaskIdHigh = 0;
  can_filter.FilterMaskIdLow = 0;
  can_filter.FilterActivation = ENABLE;
  can_filter.SlaveStartFilterBank = 14;
  can_filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;

  HAL_CAN_ConfigFilter(bsp_can_get_handle(BSP_CAN_1), &can_filter);
  HAL_CAN_Start(bsp_can_get_handle(BSP_CAN_1));
  bsp_can_register_callback(BSP_CAN_1, HAL_CAN_RX_FIFO0_MSG_PENDING_CB,
                            can_can1_rx_fifo_msg_pending_callback, NULL);
  HAL_CAN_ActivateNotification(bsp_can_get_handle(BSP_CAN_1),
                               CAN_IT_RX_FIFO0_MSG_PENDING);

  can_filter.FilterBank = 14;
  can_filter.FilterFIFOAssignment = CAN_FILTER_FIFO1;

  HAL_CAN_ConfigFilter(bsp_can_get_handle(BSP_CAN_2), &can_filter);
  HAL_CAN_Start(bsp_can_get_handle(BSP_CAN_2));
  bsp_can_register_callback(BSP_CAN_2, HAL_CAN_RX_FIFO1_MSG_PENDING_CB,
                            can_can2_rx_fifo_msg_pending_callback, NULL);
  HAL_CAN_ActivateNotification(bsp_can_get_handle(BSP_CAN_2),
                               CAN_IT_RX_FIFO1_MSG_PENDING);

  inited = true;
  return DEVICE_OK;
}
