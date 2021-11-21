#include "bsp_uart.h"

#include "comp_utils.h"

static BSP_Callback_t callback_list[BSP_UART_NUM][BSP_UART_CB_NUM];

static BSP_UART_t UART_Get(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART3)
    return BSP_UART_DR16;
  else if (huart->Instance == USART1)
    return BSP_UART_REF;
  else if (huart->Instance == USART6)
    return BSP_UART_AI;
  /*
  else if (huart->Instance == USARTX)
                  return BSP_UART_XXX;
  */
  else
    return BSP_UART_ERR;
}

static void BSP_UART_Callback(BSP_UART_Callback_t cb_type,
                              UART_HandleTypeDef *huart) {
  BSP_UART_t bsp_uart = UART_Get(huart);
  if (bsp_uart != BSP_UART_ERR) {
    BSP_Callback_t cb = callback_list[bsp_uart][cb_type];

    if (cb.Fn) {
      cb.Fn(cb.arg);
    }
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  BSP_UART_Callback(BSP_UART_TX_CPLT_CB, huart);
}

void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart) {
  BSP_UART_Callback(BSP_UART_TX_HALF_CPLT_CB, huart);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  BSP_UART_Callback(BSP_UART_RX_CPLT_CB, huart);
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
  BSP_UART_Callback(BSP_UART_RX_HALF_CPLT_CB, huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  BSP_UART_Callback(BSP_UART_ERROR_CB, huart);
}

void HAL_UART_AbortCpltCallback(UART_HandleTypeDef *huart) {
  BSP_UART_Callback(BSP_UART_ABORT_CPLT_CB, huart);
}

void HAL_UART_AbortTransmitCpltCallback(UART_HandleTypeDef *huart) {
  BSP_UART_Callback(BSP_UART_ABORT_TX_CPLT_CB, huart);
}

void HAL_UART_AbortReceiveCpltCallback(UART_HandleTypeDef *huart) {
  BSP_UART_Callback(BSP_UART_ABORT_RX_CPLT_CB, huart);
}

void BSP_UART_IRQHandler(UART_HandleTypeDef *huart) {
  if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE)) {
    __HAL_UART_CLEAR_IDLEFLAG(huart);
    BSP_UART_Callback(BSP_UART_IDLE_LINE_CB, huart);
  }
}

UART_HandleTypeDef *BSP_UART_GetHandle(BSP_UART_t uart) {
  switch (uart) {
    case BSP_UART_DR16:
      return &huart3;
    case BSP_UART_REF:
      return &huart1;
    case BSP_UART_AI:
      return &huart6;
    /*
    case BSP_UART_XXX:
            return &huartX;
    */
    default:
      return NULL;
  }
}

int8_t BSP_UART_RegisterCallback(BSP_UART_t uart, BSP_UART_Callback_t type,
                                 void (*callback)(void *), void *callback_arg) {
  ASSERT(callback);
  ASSERT(type != BSP_UART_CB_NUM);

  callback_list[uart][type].Fn = callback;
  callback_list[uart][type].arg = callback_arg;
  return BSP_OK;
}
