/* Includes ------------------------------------------------------------------*/
#include "bsp\uart.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static void (*UART_Callback[BSP_UART_NUM][BSP_UART_CB_NUM])(void);

/* Private function  ---------------------------------------------------------*/
static BSP_UART_t UART_Get(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART3)
		return BSP_UART_DR16;
	else if (huart->Instance == USART6)
			return BSP_UART_REF;
	/*
	else if (huart->Instance == USARTX)
			return BSP_UART_XXX;
	*/
	else
		return BSP_UART_NUM;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if (UART_Callback[UART_Get(huart)][BSP_UART_TX_CPLT_CB]) {
		UART_Callback[UART_Get(huart)][BSP_UART_TX_CPLT_CB]();
	}
}

void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart) {
	if (UART_Callback[UART_Get(huart)][BSP_UART_TX_HALF_CPLT_CB]) {
		UART_Callback[UART_Get(huart)][BSP_UART_TX_HALF_CPLT_CB]();
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (UART_Callback[UART_Get(huart)][BSP_UART_RX_CPLT_CB]) {
		UART_Callback[UART_Get(huart)][BSP_UART_RX_CPLT_CB]();
	}
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
	if (UART_Callback[UART_Get(huart)][BSP_UART_RX_HALF_CPLT_CB]) {
		UART_Callback[UART_Get(huart)][BSP_UART_RX_HALF_CPLT_CB]();
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
	if (UART_Callback[UART_Get(huart)][BSP_UART_ERROR_CB]) {
		UART_Callback[UART_Get(huart)][BSP_UART_ERROR_CB]();
	}
}

void HAL_UART_AbortCpltCallback(UART_HandleTypeDef *huart) {
	if (UART_Callback[UART_Get(huart)][BSP_UART_ABORT_CPLT_CB]) {
		UART_Callback[UART_Get(huart)][BSP_UART_ABORT_CPLT_CB]();
	}
}

void HAL_UART_AbortTransmitCpltCallback(UART_HandleTypeDef *huart) {
	if (UART_Callback[UART_Get(huart)][BSP_UART_ABORT_TX_CPLT_CB]) {
		UART_Callback[UART_Get(huart)][BSP_UART_ABORT_TX_CPLT_CB]();
	}
}

void HAL_UART_AbortReceiveCpltCallback(UART_HandleTypeDef *huart) {
	if (UART_Callback[UART_Get(huart)][BSP_UART_ABORT_RX_CPLT_CB]) {
		UART_Callback[UART_Get(huart)][BSP_UART_ABORT_RX_CPLT_CB]();
	}
}

/* Exported functions --------------------------------------------------------*/
void BSP_UART_IRQHandler(UART_HandleTypeDef *huart) {
	if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE)) {
		__HAL_UART_CLEAR_FLAG(huart, UART_FLAG_IDLE);
		
		if (UART_Callback[UART_Get(huart)][BSP_UART_IDLE_LINE_CB]) {
			UART_Callback[UART_Get(huart)][BSP_UART_IDLE_LINE_CB]();
		}
	}
}

UART_HandleTypeDef *BSP_UART_GetHandle(BSP_UART_t uart) {
	switch (uart) {
		case BSP_UART_DR16:
			return &huart3;
		case BSP_UART_REF:
			return &huart6;
		/*
		case BSP_UART_XXX:
			return &huartX;
		*/
		case BSP_UART_NUM:
			return NULL;
	}
}

int8_t BSP_UART_RegisterCallback(BSP_UART_t uart, BSP_UART_Callback_t type, void (*callback)(void)) {
	if (callback == NULL)
		return -1;
	UART_Callback[uart][type] = callback;
	return 0;
}
