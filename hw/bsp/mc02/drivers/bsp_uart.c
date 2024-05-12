#include "bsp_uart.h"

#include "main.h"
#include "stm32h7xx_it.h"

extern UART_HandleTypeDef huart5;
extern UART_HandleTypeDef huart7;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart10;
extern DMA_HandleTypeDef hdma_uart5_rx;
extern DMA_HandleTypeDef hdma_uart7_rx;
extern DMA_HandleTypeDef hdma_uart7_tx;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;
extern DMA_HandleTypeDef hdma_usart3_tx;
extern DMA_HandleTypeDef hdma_usart3_rx;
extern DMA_HandleTypeDef hdma_usart10_rx;
extern DMA_HandleTypeDef hdma_usart10_tx;

static bsp_callback_t callback_list[BSP_UART_NUM][BSP_UART_CB_NUM];

static bsp_uart_t uart_get(UART_HandleTypeDef *huart) {
  if (huart->Instance == UART5) {
    return BSP_UART_DR16;
  } else if (huart->Instance == USART1) {
    return BSP_UART_REF;
  } else if (huart->Instance == UART7) {
    return BSP_UART_AI;
  } else if (huart->Instance == USART10) {
    return BSP_UART_CUSTOM;
  } else if (huart->Instance == USART2) {
    return BSP_UART_RS485_1;
  } else if (huart->Instance == USART3) {
    return BSP_UART_RS485_2;
  }
  /*
  else if (huart->Instance == USARTX)
                  return BSP_UART_XXX;
  */
  else {
    return BSP_UART_ERR;
  }
}
UART_HandleTypeDef *bsp_uart_get_handle(bsp_uart_t uart) {
  switch (uart) {
    case BSP_UART_DR16:
      return &huart5;
    case BSP_UART_REF:
      return &huart1;
    case BSP_UART_AI:
      return &huart7;
    case BSP_UART_CUSTOM:
      return &huart10;
    case BSP_UART_RS485_1:
      return &huart2;
    case BSP_UART_RS485_2:
      return &huart3;
    /*
    case BSP_UART_XXX:
            return &huartX;
    */
    default:
      return NULL;
  }
}
static void bsp_uart_callback(bsp_uart_callback_t cb_type,
                              UART_HandleTypeDef *huart) {
  bsp_uart_t bsp_uart = uart_get(huart);
  if (bsp_uart != BSP_UART_ERR) {
    bsp_callback_t cb = callback_list[bsp_uart][cb_type];

    if (cb.fn) {
      cb.fn(cb.arg);
    }
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  bsp_uart_callback(BSP_UART_TX_CPLT_CB, huart);
}

void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart) {
  bsp_uart_callback(BSP_UART_TX_HALF_CPLT_CB, huart);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  bsp_uart_callback(BSP_UART_RX_CPLT_CB, huart);
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
  bsp_uart_callback(BSP_UART_RX_HALF_CPLT_CB, huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  bsp_uart_callback(BSP_UART_ERROR_CB, huart);
}

void HAL_UART_AbortCpltCallback(UART_HandleTypeDef *huart) {
  bsp_uart_callback(BSP_UART_ABORT_CPLT_CB, huart);
}

void HAL_UART_AbortTransmitCpltCallback(UART_HandleTypeDef *huart) {
  bsp_uart_callback(BSP_UART_ABORT_TX_CPLT_CB, huart);
}

void HAL_UART_AbortReceiveCpltCallback(UART_HandleTypeDef *huart) {
  bsp_uart_callback(BSP_UART_ABORT_RX_CPLT_CB, huart);
}

void bsp_uart_irq_handler(UART_HandleTypeDef *huart) {
  if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE)) {
    __HAL_UART_CLEAR_IDLEFLAG(huart);
    bsp_uart_callback(BSP_UART_IDLE_LINE_CB, huart);
  }
}

void bsp_uart_init() {
  HAL_UART_RegisterUserCallback(bsp_uart_irq_handler);
  __HAL_UART_ENABLE_IT(bsp_uart_get_handle(BSP_UART_REF), UART_IT_IDLE);
  __HAL_UART_ENABLE_IT(bsp_uart_get_handle(BSP_UART_AI), UART_IT_IDLE);
  __HAL_UART_ENABLE_IT(bsp_uart_get_handle(BSP_UART_CUSTOM), UART_IT_IDLE);
  __HAL_UART_ENABLE_IT(bsp_uart_get_handle(BSP_UART_RS485_1), UART_IT_IDLE);
  __HAL_UART_ENABLE_IT(bsp_uart_get_handle(BSP_UART_RS485_2), UART_IT_IDLE);
}

bsp_status_t bsp_uart_register_callback(bsp_uart_t uart,
                                        bsp_uart_callback_t type,
                                        void (*callback)(void *),
                                        void *callback_arg) {
  assert_param(callback);
  assert_param(type != BSP_UART_CB_NUM);

  callback_list[uart][type].fn = callback;
  callback_list[uart][type].arg = callback_arg;
  return BSP_OK;
}

bsp_status_t bsp_uart_abort_receive(bsp_uart_t uart) {
  HAL_UART_AbortReceive_IT(bsp_uart_get_handle(uart));

  switch (uart) {
    case BSP_UART_DR16: {
      __HAL_DMA_SET_COUNTER(&hdma_uart5_rx, 0);
      break;
    }
    case BSP_UART_REF: {
      __HAL_DMA_SET_COUNTER(&hdma_usart1_rx, 0);
      break;
    }
    case BSP_UART_AI: {
      __HAL_DMA_SET_COUNTER(&hdma_uart7_rx, 0);
      break;
    }
    case BSP_UART_CUSTOM: {
      __HAL_DMA_SET_COUNTER(&hdma_usart10_rx, 0);
      break;
    }
    case BSP_UART_RS485_1: {
      __HAL_DMA_SET_COUNTER(&hdma_usart2_rx, 0);
      break;
    }
    case BSP_UART_RS485_2: {
      __HAL_DMA_SET_COUNTER(&hdma_usart3_rx, 0);
      break;
    }
    default:
      return BSP_ERR;
  }
  return BSP_OK;
}

bsp_status_t bsp_uart_abort_transmit(bsp_uart_t uart) {
  HAL_UART_AbortTransmit_IT(bsp_uart_get_handle(uart));

  switch (uart) {
    case BSP_UART_DR16: {
      return BSP_ERR;
      break;
    }
    case BSP_UART_REF: {
      __HAL_DMA_SET_COUNTER(&hdma_usart1_tx, 0);
      break;
    }
    case BSP_UART_AI: {
      __HAL_DMA_SET_COUNTER(&hdma_uart7_tx, 0);
      break;
    }
    case BSP_UART_CUSTOM: {
      __HAL_DMA_SET_COUNTER(&hdma_usart10_tx, 0);
      break;
    }
    case BSP_UART_RS485_1: {
      __HAL_DMA_SET_COUNTER(&hdma_usart2_tx, 0);
      break;
    }
    case BSP_UART_RS485_2: {
      __HAL_DMA_SET_COUNTER(&hdma_usart3_tx, 0);
      break;
    }
    default:
      return BSP_ERR;
  }

  return BSP_OK;
}

bsp_status_t bsp_uart_transmit(bsp_uart_t uart, uint8_t *data, size_t size,
                               bool block) {
  if (block) {
    return HAL_UART_Transmit(bsp_uart_get_handle(uart), data, size, 10) !=
           HAL_OK;
  } else {
    return HAL_UART_Transmit_DMA(bsp_uart_get_handle(uart), data, size) !=
           HAL_OK;
  }
}

bsp_status_t bsp_uart_receive(bsp_uart_t uart, uint8_t *buff, size_t size,
                              bool block) {
  if (block) {
    return HAL_UART_Receive(bsp_uart_get_handle(uart), buff, size, 10) !=
           HAL_OK;
  } else {
    return HAL_UART_Receive_DMA(bsp_uart_get_handle(uart), buff, size) !=
           HAL_OK;
  }
}

uint32_t bsp_uart_get_count(bsp_uart_t uart) {
  return bsp_uart_get_handle(uart)->RxXferSize -
         __HAL_DMA_GET_COUNTER(bsp_uart_get_handle(uart)->hdmarx);
}
