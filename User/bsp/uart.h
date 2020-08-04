#pragma once


/* Includes ------------------------------------------------------------------*/
#include <usart.h>

#include "bsp/bsp.h"

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum {
	BSP_UART_DR16,
	BSP_UART_REF,
	/* BSP_UART_XXX, */
	BSP_UART_NUM,
} BSP_UART_t;

typedef enum {
	BSP_UART_TX_HALF_CPLT_CB,
	BSP_UART_TX_CPLT_CB,
	BSP_UART_RX_HALF_CPLT_CB,
	BSP_UART_RX_CPLT_CB,
	BSP_UART_ERROR_CB,
	BSP_UART_ABORT_CPLT_CB,
	BSP_UART_ABORT_TX_CPLT_CB,
	BSP_UART_ABORT_RX_CPLT_CB,
	
	BSP_UART_IDLE_LINE_CB,
	BSP_UART_CB_NUM,
} BSP_UART_Callback_t;

/* Exported functions prototypes ---------------------------------------------*/
void BSP_UART_IRQHandler(UART_HandleTypeDef *huart);
UART_HandleTypeDef *BSP_UART_GetHandle(BSP_UART_t uart);
int8_t BSP_UART_RegisterCallback(BSP_UART_t uart, BSP_UART_Callback_t type, void (*callback)(void));
