#include "dev_can.h"

#include <stdbool.h>
#include <string.h>

#include "comp_utils.h"

static CAN_RawRx_t raw_rx1, raw_rx2;

static bool inited = false;

static void CAN_CAN1RxFifoMsgPendingCallback(void* arg) {
  UNUSED(arg);

  HAL_CAN_GetRxMessage(BSP_CAN_GetHandle(BSP_CAN_1), CAN_FILTER_FIFO0,
                       &raw_rx1.header, raw_rx1.data);
  BSP_CAN_PublishData(BSP_CAN_1, &raw_rx1);
}

static void CAN_CAN2RxFifoMsgPendingCallback(void* arg) {
  UNUSED(arg);

  HAL_CAN_GetRxMessage(BSP_CAN_GetHandle(BSP_CAN_2), CAN_FILTER_FIFO1,
                       &raw_rx2.header, raw_rx2.data);
  BSP_CAN_PublishData(BSP_CAN_2, &raw_rx2);
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

  HAL_CAN_ConfigFilter(BSP_CAN_GetHandle(BSP_CAN_1), &can_filter);
  HAL_CAN_Start(BSP_CAN_GetHandle(BSP_CAN_1));
  BSP_CAN_RegisterCallback(BSP_CAN_1, HAL_CAN_RX_FIFO0_MSG_PENDING_CB,
                           CAN_CAN1RxFifoMsgPendingCallback, NULL);
  HAL_CAN_ActivateNotification(BSP_CAN_GetHandle(BSP_CAN_1),
                               CAN_IT_RX_FIFO0_MSG_PENDING);

  can_filter.FilterBank = 14;
  can_filter.FilterFIFOAssignment = CAN_FILTER_FIFO1;

  HAL_CAN_ConfigFilter(BSP_CAN_GetHandle(BSP_CAN_2), &can_filter);
  HAL_CAN_Start(BSP_CAN_GetHandle(BSP_CAN_2));
  BSP_CAN_RegisterCallback(BSP_CAN_2, HAL_CAN_RX_FIFO1_MSG_PENDING_CB,
                           CAN_CAN2RxFifoMsgPendingCallback, NULL);
  HAL_CAN_ActivateNotification(BSP_CAN_GetHandle(BSP_CAN_2),
                               CAN_IT_RX_FIFO1_MSG_PENDING);

  inited = true;
  return DEVICE_OK;
}
