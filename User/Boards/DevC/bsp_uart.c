/* Includes ------------------------------------------------------------------*/
#include "bsp_uart.h"

#include "usart.h"

/* Private define ------------------------------------------------------------*/
#define DR16_UART USART3
/* #define XXX_UART USARTX */

/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
struct {
	struct {
		void(*TxHalfCpltCallback)(void);        /* UART Tx Half Complete Callback        */
		void(*TxCpltCallback)(void);            /* UART Tx Complete Callback             */
		void(*RxHalfCpltCallback)(void);        /* UART Rx Half Complete Callback        */
		void(*RxCpltCallback)(void);            /* UART Rx Complete Callback             */
		void(*ErrorCallback)(void);             /* UART Error Callback                   */
		void(*AbortCpltCallback)(void);         /* UART Abort Complete Callback          */
		void(*AbortTransmitCpltCallback)(void); /* UART Abort Transmit Complete Callback */
		void(*AbortReceiveCpltCallback)(void);  /* UART Abort Receive Complete Callback  */
	} dr16;
	
	/*
	struct {
		void(*TxHalfCpltCallback)(void);
		void(*TxCpltCallback)(void);
		void(*RxHalfCpltCallback)(void);
		void(*RxCpltCallback)(void);
		void(*ErrorCallback)(void);
		void(*AbortCpltCallback)(void);
		void(*AbortTransmitCpltCallback)(void);
		void(*AbortReceiveCpltCallback)(void);
	} xx;
	*/
} static bsp_uart_callback;

/* Private function  ---------------------------------------------------------*/
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if(huart->Instance == DR16_UART) {
		if (bsp_uart_callback.dr16.TxCpltCallback) {
			bsp_uart_callback.dr16.TxCpltCallback();
		}
	}
}

void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart) {
	if(huart->Instance == DR16_UART) {
		if (bsp_uart_callback.dr16.TxHalfCpltCallback) {
			bsp_uart_callback.dr16.TxHalfCpltCallback();
		}
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if(huart->Instance == DR16_UART) {
		if (bsp_uart_callback.dr16.RxCpltCallback) {
			bsp_uart_callback.dr16.RxCpltCallback();
		}
	}
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
	if(huart->Instance == DR16_UART) {
		if (bsp_uart_callback.dr16.RxHalfCpltCallback) {
			bsp_uart_callback.dr16.RxHalfCpltCallback();
		}
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
	if(huart->Instance == DR16_UART) {
		if (bsp_uart_callback.dr16.ErrorCallback) {
			bsp_uart_callback.dr16.ErrorCallback();
		}
	}
}

void HAL_UART_AbortCpltCallback(UART_HandleTypeDef *huart) {
	if(huart->Instance == DR16_UART) {
		if (bsp_uart_callback.dr16.AbortCpltCallback) {
			bsp_uart_callback.dr16.AbortCpltCallback();
		}
	}
}

void HAL_UART_AbortTransmitCpltCallback(UART_HandleTypeDef *huart) {
	if(huart->Instance == DR16_UART) {
		if (bsp_uart_callback.dr16.AbortTransmitCpltCallback) {
			bsp_uart_callback.dr16.AbortTransmitCpltCallback();
		}
	}
}

void HAL_UART_AbortReceiveCpltCallback(UART_HandleTypeDef *huart) {
	if(huart->Instance == DR16_UART) {
		if (bsp_uart_callback.dr16.AbortReceiveCpltCallback) {
			bsp_uart_callback.dr16.AbortReceiveCpltCallback();
		}
	}
	/* 
	else if (hspi->Instance == XXX_UART) {
		if (bsp_uart_callback.xxx.AbortReceiveCpltCallback) {
			bsp_uart_callback.xxx.AbortReceiveCpltCallback();
		}
	}
	*/
}

/* Exported functions --------------------------------------------------------*/
int8_t BSP_UART_RegisterCallback(BSP_UART_t uart, BSP_UART_Callback_t type, void (*callback)(void)) {
	if (callback == NULL)
		return -1;
	
	switch (uart) {
		case BSP_UART_DR16:
			switch (type) {
				case BSP_UART_TX_HALFCOMPLETE_CB:
					bsp_uart_callback.dr16.TxHalfCpltCallback = callback;
					break;
				case BSP_UART_TX_COMPLETE_CB:
					bsp_uart_callback.dr16.TxCpltCallback = callback;
					break;
				case BSP_UART_RX_HALFCOMPLETE_CB:
					bsp_uart_callback.dr16.RxHalfCpltCallback = callback;
					break;
				case BSP_UART_RX_COMPLETE_CB:
					bsp_uart_callback.dr16.RxCpltCallback = callback;
					break;
				case BSP_UART_ERROR_CB:
					bsp_uart_callback.dr16.ErrorCallback = callback;
					break;
				case BSP_UART_ABORT_COMPLETE_CB:
					bsp_uart_callback.dr16.AbortCpltCallback = callback;
					break;
				case BSP_UART_ABORT_TRANSMIT_COMPLETE_CB:
					bsp_uart_callback.dr16.AbortTransmitCpltCallback = callback;
					break;
				case BSP_UART_ABORT_RECEIVE_COMPLETE_CB:
					bsp_uart_callback.dr16.AbortReceiveCpltCallback = callback;
					break;
				default:
					return -1;
			}
			break;
		/*
		case BSP_UART_XXX:
			switch (type) {
				case BSP_UART_TX_HALFCOMPLETE_CB:
					bsp_uart_callback.xxx.TxHalfCpltCallback = callback;
					break;
				case BSP_UART_TX_COMPLETE_CB:
					bsp_uart_callback.xxx.TxCpltCallback = callback;
					break;
				case BSP_UART_RX_HALFCOMPLETE_CB:
					bsp_uart_callback.xxx.RxHalfCpltCallback = callback;
					break;
				case BSP_UART_RX_COMPLETE_CB:
					bsp_uart_callback.xxx.RxCpltCallback = callback;
					break;
				case BSP_UART_ERROR_CB:
					bsp_uart_callback.xxx.ErrorCallback = callback;
					break;
				case BSP_UART_ABORT_COMPLETE_CB:
					bsp_uart_callback.xxx.AbortCpltCallback = callback;
					break;
				case BSP_UART_ABORT_TRANSMIT_COMPLETE_CB:
					bsp_uart_callback.xxx.AbortTransmitCpltCallback = callback;
					break;
				case BSP_UART_ABORT_RECEIVE_COMPLETE_CB:
					bsp_uart_callback.xxx.AbortReceiveCpltCallback = callback;
					break;
				default:
					return -1;
			}
			break;
		*/
		default:
			return -1;
	}
	return 0;
}

int8_t BSP_UART_Transmit(BSP_UART_t uart, uint8_t *data, uint16_t len, uint32_t time_out) {
	if (data == NULL)
		return -1;
	
	switch (uart) {
		case BSP_UART_DR16:
			return -1;
		/*
		case BSP_UART_XXX:
			return -1;
		*/
		default:
			return -1;
	}
}

int8_t BSP_UART_TransmitDMA(BSP_UART_t uart, uint8_t *data, uint16_t len) {
	if (data == NULL)
		return -1;
	
	switch (uart) {
		case BSP_UART_DR16:
			return -1;
		/*
		case BSP_UART_XXX:
			return -1;
		*/
		default:
			return -1;
	}
}

int8_t BSP_UART_Receive(BSP_UART_t uart, uint8_t *data, uint16_t len, uint32_t time_out) {
	if (data == NULL)
		return -1;

	switch (uart) {
		case BSP_UART_DR16:
			return HAL_UART_Receive(&huart1, data, len, time_out);
		/*
		case BSP_UART_XXX:
			return -1;
		*/
		default:
			return -1;
	}
}

int8_t BSP_UART_ReceiveDMA(BSP_UART_t uart, uint8_t *data, uint16_t len) {
	if (data == NULL)
		return -1;

	switch (uart) {
		case BSP_UART_DR16:
			return HAL_UART_Receive_DMA(&huart1, data, len);
		/*
		case BSP_UART_XXX:
			return -1;
		*/
		default:
			return -1;
	}
}
