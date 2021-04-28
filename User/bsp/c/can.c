/* Includes ----------------------------------------------------------------- */
#include "bsp/can.h"

#include "component/user_math.h"

/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private typedef ---------------------------------------------------------- */
/* Private variables -------------------------------------------------------- */
static void (*CAN_Callback[BSP_CAN_NUM][BSP_CAN_CB_NUM])(void);

/* Private function  -------------------------------------------------------- */
static BSP_CAN_t CAN_Get(CAN_HandleTypeDef *hcan) {
  if (hcan->Instance == CAN2)
    return BSP_CAN_2;
  else if (hcan->Instance == CAN1)
    return BSP_CAN_1;
  else
    return BSP_CAN_ERR;
}

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_t bsp_can = CAN_Get(hcan);
  if (bsp_can != BSP_CAN_ERR) {
    if (CAN_Callback[bsp_can][HAL_CAN_TX_MAILBOX0_CPLT_CB])
      CAN_Callback[bsp_can][HAL_CAN_TX_MAILBOX0_CPLT_CB]();
  }
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_t bsp_can = CAN_Get(hcan);
  if (bsp_can != BSP_CAN_ERR) {
    if (CAN_Callback[bsp_can][HAL_CAN_TX_MAILBOX1_CPLT_CB])
      CAN_Callback[bsp_can][HAL_CAN_TX_MAILBOX1_CPLT_CB]();
  }
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_t bsp_can = CAN_Get(hcan);
  if (bsp_can != BSP_CAN_ERR) {
    if (CAN_Callback[bsp_can][HAL_CAN_TX_MAILBOX2_CPLT_CB])
      CAN_Callback[bsp_can][HAL_CAN_TX_MAILBOX2_CPLT_CB]();
  }
}

void HAL_CAN_TxMailbox0AbortCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_t bsp_can = CAN_Get(hcan);
  if (bsp_can != BSP_CAN_ERR) {
    if (CAN_Callback[bsp_can][HAL_CAN_TX_MAILBOX0_ABORT_CB])
      CAN_Callback[bsp_can][HAL_CAN_TX_MAILBOX0_ABORT_CB]();
  }
}

void HAL_CAN_TxMailbox1AbortCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_t bsp_can = CAN_Get(hcan);
  if (bsp_can != BSP_CAN_ERR) {
    if (CAN_Callback[bsp_can][HAL_CAN_TX_MAILBOX1_ABORT_CB])
      CAN_Callback[bsp_can][HAL_CAN_TX_MAILBOX1_ABORT_CB]();
  }
}

void HAL_CAN_TxMailbox2AbortCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_t bsp_can = CAN_Get(hcan);
  if (bsp_can != BSP_CAN_ERR) {
    if (CAN_Callback[bsp_can][HAL_CAN_TX_MAILBOX2_ABORT_CB])
      CAN_Callback[bsp_can][HAL_CAN_TX_MAILBOX2_ABORT_CB]();
  }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_t bsp_can = CAN_Get(hcan);
  if (bsp_can != BSP_CAN_ERR) {
    if (CAN_Callback[bsp_can][HAL_CAN_RX_FIFO0_MSG_PENDING_CB])
      CAN_Callback[bsp_can][HAL_CAN_RX_FIFO0_MSG_PENDING_CB]();
  }
}

void HAL_CAN_RxFifo0FullCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_t bsp_can = CAN_Get(hcan);
  if (bsp_can != BSP_CAN_ERR) {
    if (CAN_Callback[bsp_can][HAL_CAN_RX_FIFO0_FULL_CB])
      CAN_Callback[bsp_can][HAL_CAN_RX_FIFO0_FULL_CB]();
  }
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_t bsp_can = CAN_Get(hcan);
  if (bsp_can != BSP_CAN_ERR) {
    if (CAN_Callback[bsp_can][HAL_CAN_RX_FIFO1_MSG_PENDING_CB])
      CAN_Callback[bsp_can][HAL_CAN_RX_FIFO1_MSG_PENDING_CB]();
  }
}

void HAL_CAN_RxFifo1FullCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_t bsp_can = CAN_Get(hcan);
  if (bsp_can != BSP_CAN_ERR) {
    if (CAN_Callback[bsp_can][HAL_CAN_RX_FIFO1_FULL_CB])
      CAN_Callback[bsp_can][HAL_CAN_RX_FIFO1_FULL_CB]();
  }
}

void HAL_CAN_SleepCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_t bsp_can = CAN_Get(hcan);
  if (bsp_can != BSP_CAN_ERR) {
    if (CAN_Callback[bsp_can][HAL_CAN_SLEEP_CB])
      CAN_Callback[bsp_can][HAL_CAN_SLEEP_CB]();
  }
}

void HAL_CAN_WakeUpFromRxMsgCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_t bsp_can = CAN_Get(hcan);
  if (bsp_can != BSP_CAN_ERR) {
    if (CAN_Callback[bsp_can][HAL_CAN_WAKEUP_FROM_RX_MSG_CB])
      CAN_Callback[bsp_can][HAL_CAN_WAKEUP_FROM_RX_MSG_CB]();
  }
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) {
  BSP_CAN_t bsp_can = CAN_Get(hcan);
  if (bsp_can != BSP_CAN_ERR) {
    if (CAN_Callback[bsp_can][HAL_CAN_ERROR_CB])
      CAN_Callback[bsp_can][HAL_CAN_ERROR_CB]();
  }
}

/* Exported functions ------------------------------------------------------- */
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
                                void (*callback)(void)) {
  ASSERT(callback);
  CAN_Callback[can][type] = callback;
  return BSP_OK;
}
