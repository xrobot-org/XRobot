#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <can.h>

#include "bsp/bsp.h"

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
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

/* Exported functions prototypes ---------------------------------------------*/
CAN_HandleTypeDef *BSP_CAN_GetHandle(BSP_CAN_t can);
int8_t BSP_CAN_RegisterCallback(BSP_CAN_t can, BSP_CAN_Callback_t type,
                                void (*callback)(void));

#ifdef __cplusplus
}
#endif
