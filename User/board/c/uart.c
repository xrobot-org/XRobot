/* Includes ------------------------------------------------------------------*/
#include "board\uart.h"

#include "usart.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
struct {
	struct {
		void(*TxHalfCpltCallback)(void);		/* UART Tx Half Complete Callback */
		void(*TxCpltCallback)(void);			/* UART Tx Complete Callback */
		void(*RxHalfCpltCallback)(void);		/* UART Rx Half Complete Callback */
		void(*RxCpltCallback)(void);			/* UART Rx Complete Callback */
		void(*ErrorCallback)(void);				/* UART Error Callback */
		void(*AbortCpltCallback)(void);			/* UART Abort Complete Callback */
		void(*AbortTransmitCpltCallback)(void);	/* UART Abort Transmit Complete Callback */
		void(*AbortReceiveCpltCallback)(void);	/* UART Abort Receive Complete Callback  */
	} dr16;
	
	struct {
		void(*TxHalfCpltCallback)(void);		/* UART Tx Half Complete Callback */
		void(*TxCpltCallback)(void);			/* UART Tx Complete Callback */
		void(*RxHalfCpltCallback)(void);		/* UART Rx Half Complete Callback */
		void(*RxCpltCallback)(void);			/* UART Rx Complete Callback */
		void(*ErrorCallback)(void);				/* UART Error Callback */
		void(*AbortCpltCallback)(void);			/* UART Abort Complete Callback */
		void(*AbortTransmitCpltCallback)(void);	/* UART Abort Transmit Complete Callback */
		void(*AbortReceiveCpltCallback)(void);	/* UART Abort Receive Complete Callback  */
	} ref;
	
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
	} xxx;
	*/
} static bsp_uart_callback;

/* Private function  ---------------------------------------------------------*/
static USART_TypeDef *UART_GetInstance(BSP_UART_t uart) {
	switch (uart) {
		case BSP_UART_DR16:
			return USART3;
		case BSP_UART_REF:
			return USART6;
		/*
		case BSP_UART_XXX:
			return USARTX;
		*/
		default:
			return NULL;
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == UART_GetInstance(BSP_UART_DR16)) {
		if (bsp_uart_callback.dr16.TxCpltCallback) {
			bsp_uart_callback.dr16.TxCpltCallback();
		}
	} else if (huart->Instance == UART_GetInstance(BSP_UART_REF)) {
		if (bsp_uart_callback.ref.TxCpltCallback) {
			bsp_uart_callback.ref.TxCpltCallback();
		}
	}
}

void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == UART_GetInstance(BSP_UART_DR16)) {
		if (bsp_uart_callback.dr16.TxHalfCpltCallback) {
			bsp_uart_callback.dr16.TxHalfCpltCallback();
		}
	} else if (huart->Instance == UART_GetInstance(BSP_UART_REF)) {
		if (bsp_uart_callback.ref.TxHalfCpltCallback) {
			bsp_uart_callback.ref.TxHalfCpltCallback();
		}
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == UART_GetInstance(BSP_UART_DR16)) {
		if (bsp_uart_callback.dr16.RxCpltCallback) {
			bsp_uart_callback.dr16.RxCpltCallback();
		}
	} else if (huart->Instance == UART_GetInstance(BSP_UART_REF)) {
		if (bsp_uart_callback.ref.RxCpltCallback) {
			bsp_uart_callback.ref.RxCpltCallback();
		}
	}
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == UART_GetInstance(BSP_UART_DR16)) {
		if (bsp_uart_callback.dr16.RxHalfCpltCallback) {
			bsp_uart_callback.dr16.RxHalfCpltCallback();
		}
	} else if (huart->Instance == UART_GetInstance(BSP_UART_REF)) {
		if (bsp_uart_callback.ref.RxHalfCpltCallback) {
			bsp_uart_callback.ref.RxHalfCpltCallback();
		}
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == UART_GetInstance(BSP_UART_DR16)) {
		if (bsp_uart_callback.dr16.ErrorCallback) {
			bsp_uart_callback.dr16.ErrorCallback();
		}
	} else if (huart->Instance == UART_GetInstance(BSP_UART_REF)) {
		if (bsp_uart_callback.ref.ErrorCallback) {
			bsp_uart_callback.ref.ErrorCallback();
		}
	}
}

void HAL_UART_AbortCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == UART_GetInstance(BSP_UART_DR16)) {
		if (bsp_uart_callback.dr16.AbortCpltCallback) {
			bsp_uart_callback.dr16.AbortCpltCallback();
		}
	} else if (huart->Instance == UART_GetInstance(BSP_UART_REF)) {
		if (bsp_uart_callback.ref.AbortCpltCallback) {
			bsp_uart_callback.ref.AbortCpltCallback();
		}
	}
}

void HAL_UART_AbortTransmitCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == UART_GetInstance(BSP_UART_DR16)) {
		if (bsp_uart_callback.dr16.AbortTransmitCpltCallback) {
			bsp_uart_callback.dr16.AbortTransmitCpltCallback();
		}
	} else if (huart->Instance == UART_GetInstance(BSP_UART_REF)) {
		if (bsp_uart_callback.ref.AbortTransmitCpltCallback) {
			bsp_uart_callback.ref.AbortTransmitCpltCallback();
		}
	}
}

void HAL_UART_AbortReceiveCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == UART_GetInstance(BSP_UART_DR16)) {
		if (bsp_uart_callback.dr16.AbortReceiveCpltCallback) {
			bsp_uart_callback.dr16.AbortReceiveCpltCallback();
		}
	} else if (huart->Instance == UART_GetInstance(BSP_UART_REF)) {
		if (bsp_uart_callback.ref.AbortReceiveCpltCallback) {
			bsp_uart_callback.ref.AbortReceiveCpltCallback();
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
		default:
			return NULL;
	}
}

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
			
		case BSP_UART_REF:
			switch (type) {
				case BSP_UART_TX_HALFCOMPLETE_CB:
					bsp_uart_callback.ref.TxHalfCpltCallback = callback;
					break;
				case BSP_UART_TX_COMPLETE_CB:
					bsp_uart_callback.ref.TxCpltCallback = callback;
					break;
				case BSP_UART_RX_HALFCOMPLETE_CB:
					bsp_uart_callback.ref.RxHalfCpltCallback = callback;
					break;
				case BSP_UART_RX_COMPLETE_CB:
					bsp_uart_callback.ref.RxCpltCallback = callback;
					break;
				case BSP_UART_ERROR_CB:
					bsp_uart_callback.ref.ErrorCallback = callback;
					break;
				case BSP_UART_ABORT_COMPLETE_CB:
					bsp_uart_callback.ref.AbortCpltCallback = callback;
					break;
				case BSP_UART_ABORT_TRANSMIT_COMPLETE_CB:
					bsp_uart_callback.ref.AbortTransmitCpltCallback = callback;
					break;
				case BSP_UART_ABORT_RECEIVE_COMPLETE_CB:
					bsp_uart_callback.ref.AbortReceiveCpltCallback = callback;
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
