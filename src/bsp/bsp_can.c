#include "bsp_can.h"

#include "comp_utils.h"

static BSP_Callback_t callback_list[BSP_CAN_NUM][BSP_CAN_CB_NUM];

static BSP_CAN_t CAN_Get(CAN_HandleTypeDef *hcan) {
  if (hcan->Instance == CAN2)
    return BSP_CAN_2;
  else if (hcan->Instance == CAN1)
    return BSP_CAN_1;
  else
    return BSP_CAN_ERR;
}

static void BSP_CAN_Callback(BSP_CAN_Callback_t cb_type,
                             CAN_HandleTypeDef *hcan) {
  BSP_CAN_t bsp_can = CAN_Get(hcan);
  if (bsp_can != BSP_CAN_ERR) {
    BSP_Callback_t cb = callback_list[bsp_can][cb_type];

    if (cb.Fn) {
      cb.Fn(cb.arg);
    }
  }
}

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_Callback(HAL_CAN_TX_MAILBOX0_CPLT_CB, hcan);
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_Callback(HAL_CAN_TX_MAILBOX1_CPLT_CB, hcan);
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_Callback(HAL_CAN_TX_MAILBOX2_CPLT_CB, hcan);
}

void HAL_CAN_TxMailbox0AbortCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_Callback(HAL_CAN_TX_MAILBOX0_ABORT_CB, hcan);
}

void HAL_CAN_TxMailbox1AbortCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_Callback(HAL_CAN_TX_MAILBOX1_ABORT_CB, hcan);
}

void HAL_CAN_TxMailbox2AbortCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_Callback(HAL_CAN_TX_MAILBOX2_ABORT_CB, hcan);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_Callback(HAL_CAN_RX_FIFO0_MSG_PENDING_CB, hcan);
}

void HAL_CAN_RxFifo0FullCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_Callback(HAL_CAN_RX_FIFO0_FULL_CB, hcan);
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_Callback(HAL_CAN_RX_FIFO1_MSG_PENDING_CB, hcan);
}

void HAL_CAN_RxFifo1FullCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_Callback(HAL_CAN_RX_FIFO1_FULL_CB, hcan);
}

void HAL_CAN_SleepCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_Callback(HAL_CAN_SLEEP_CB, hcan);
}

void HAL_CAN_WakeUpFromRxMsgCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_Callback(HAL_CAN_WAKEUP_FROM_RX_MSG_CB, hcan);
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_Callback(HAL_CAN_ERROR_CB, hcan);
}

CAN_HandleTypeDef *BSP_CAN_GetHandle(BSP_CAN_t can) {
  switch (can) {
    case BSP_CAN_2:
      return &hcan2;
    case BSP_CAN_1:
      return &hcan1;
    default:
      return NULL;
  }
}

int8_t BSP_CAN_RegisterCallback(BSP_CAN_t can, BSP_CAN_Callback_t type,
                                void (*callback)(void *), void *callback_arg) {
  ASSERT(callback);
  ASSERT(type != BSP_CAN_CB_NUM);

  callback_list[can][type].Fn = callback;
  callback_list[can][type].arg = callback_arg;
  return BSP_OK;
}
