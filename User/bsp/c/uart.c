/* Includes ------------------------------------------------------------------*/
#include "bsp\uart.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
struct {
	struct {
		void(*TxHalfCpltCallback)(void);
		void(*TxCpltCallback)(void);
		void(*RxHalfCpltCallback)(void);
		void(*RxCpltCallback)(void);
		void(*ErrorCallback)(void);
		void(*AbortCpltCallback)(void);
		void(*AbortTxCpltCallback)(void);
		void(*AbortRxCpltCallback)(void);
		void(*IdleLineCallback)(void);
	} dr16;
	
	struct {
		void(*TxHalfCpltCallback)(void);
		void(*TxCpltCallback)(void);
		void(*RxHalfCpltCallback)(void);
		void(*RxCpltCallback)(void);
		void(*ErrorCallback)(void);
		void(*AbortCpltCallback)(void);
		void(*AbortTxCpltCallback)(void);
		void(*AbortRxCpltCallback)(void);
		void(*IdleLineCallback)(void);
	} ref;
	
	/*
	struct {
		void(*TxHalfCpltCallback)(void);
		void(*TxCpltCallback)(void);
		void(*RxHalfCpltCallback)(void);
		void(*RxCpltCallback)(void);
		void(*ErrorCallback)(void);
		void(*AbortCpltCallback)(void);
		void(*AbortTxCpltCallback)(void);
		void(*AbortRxCpltCallback)(void);
		void(*IdleLineCallback)(void);
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
		if (bsp_uart_callback.dr16.AbortTxCpltCallback) {
			bsp_uart_callback.dr16.AbortTxCpltCallback();
		}
	} else if (huart->Instance == UART_GetInstance(BSP_UART_REF)) {
		if (bsp_uart_callback.ref.AbortTxCpltCallback) {
			bsp_uart_callback.ref.AbortTxCpltCallback();
		}
	}
}

void HAL_UART_AbortReceiveCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == UART_GetInstance(BSP_UART_DR16)) {
		if (bsp_uart_callback.dr16.AbortRxCpltCallback) {
			bsp_uart_callback.dr16.AbortRxCpltCallback();
		}
	} else if (huart->Instance == UART_GetInstance(BSP_UART_REF)) {
		if (bsp_uart_callback.ref.AbortRxCpltCallback) {
			bsp_uart_callback.ref.AbortRxCpltCallback();
		}
	}
	/* 
	else if (hspi->Instance == XXX_UART) {
		if (bsp_uart_callback.xxx.AbortRxCpltCallback) {
			bsp_uart_callback.xxx.AbortRxCpltCallback();
		}
	}
	*/
}

/* Exported functions --------------------------------------------------------*/
void BSP_UART_IRQHandler(UART_HandleTypeDef *huart) {
	if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE)) {
		__HAL_UART_CLEAR_FLAG(huart, UART_FLAG_IDLE);
		if (huart->Instance == UART_GetInstance(BSP_UART_DR16)) {
			if (bsp_uart_callback.dr16.IdleLineCallback) {
				bsp_uart_callback.dr16.IdleLineCallback();
			}
		} else if (huart->Instance == UART_GetInstance(BSP_UART_REF)) {
			if (bsp_uart_callback.ref.IdleLineCallback) {
				bsp_uart_callback.ref.IdleLineCallback();
			}
		}
		/* 
		else if (hspi->Instance == XXX_UART) {
			if (bsp_uart_callback.xxx.IdleLineCallback) {
				bsp_uart_callback.xxx.IdleLineCallback();
			}
		}
		*/
		return;
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
	}
}

int8_t BSP_UART_RegisterCallback(BSP_UART_t uart, BSP_UART_Callback_t type, void (*callback)(void)) {
	if (callback == NULL)
		return -1;
	
	switch (uart) {
		case BSP_UART_DR16:
			switch (type) {
				case BSP_UART_TX_HALF_CPLT_CB:
					bsp_uart_callback.dr16.TxHalfCpltCallback = callback;
					break;
				case BSP_UART_TX_CPLT_CB:
					bsp_uart_callback.dr16.TxCpltCallback = callback;
					break;
				case BSP_UART_RX_HALF_CPLT_CB:
					bsp_uart_callback.dr16.RxHalfCpltCallback = callback;
					break;
				case BSP_UART_RX_CPLT_CB:
					bsp_uart_callback.dr16.RxCpltCallback = callback;
					break;
				case BSP_UART_ERROR_CB:
					bsp_uart_callback.dr16.ErrorCallback = callback;
					break;
				case BSP_UART_ABORT_CPLT_CB:
					bsp_uart_callback.dr16.AbortCpltCallback = callback;
					break;
				case BSP_UART_ABORT_TX_CPLT_CB:
					bsp_uart_callback.dr16.AbortTxCpltCallback = callback;
					break;
				case BSP_UART_ABORT_RX_CPLT_CB:
					bsp_uart_callback.dr16.AbortRxCpltCallback = callback;
					break;
				case BSP_UART_IDLE_LINE_CB:
					bsp_uart_callback.dr16.IdleLineCallback = callback;
					break;
			}
			break;
			
		case BSP_UART_REF:
			switch (type) {
				case BSP_UART_TX_HALF_CPLT_CB:
					bsp_uart_callback.ref.TxHalfCpltCallback = callback;
					break;
				case BSP_UART_TX_CPLT_CB:
					bsp_uart_callback.ref.TxCpltCallback = callback;
					break;
				case BSP_UART_RX_HALF_CPLT_CB:
					bsp_uart_callback.ref.RxHalfCpltCallback = callback;
					break;
				case BSP_UART_RX_CPLT_CB:
					bsp_uart_callback.ref.RxCpltCallback = callback;
					break;
				case BSP_UART_ERROR_CB:
					bsp_uart_callback.ref.ErrorCallback = callback;
					break;
				case BSP_UART_ABORT_CPLT_CB:
					bsp_uart_callback.ref.AbortCpltCallback = callback;
					break;
				case BSP_UART_ABORT_TX_CPLT_CB:
					bsp_uart_callback.ref.AbortTxCpltCallback = callback;
					break;
				case BSP_UART_ABORT_RX_CPLT_CB:
					bsp_uart_callback.ref.AbortRxCpltCallback = callback;
					break;
				case BSP_UART_IDLE_LINE_CB:
					bsp_uart_callback.ref.IdleLineCallback = callback;
			}
			break;
		/*
		case BSP_UART_XXX:
			switch (type) {
				case BSP_UART_TX_HALF_CPLT_CB:
					bsp_uart_callback.xxx.TxHalfCpltCallback = callback;
					break;
				case BSP_UART_TX_CPLT_CB:
					bsp_uart_callback.xxx.TxCpltCallback = callback;
					break;
				case BSP_UART_RX_HALF_CPLT_CB:
					bsp_uart_callback.xxx.RxHalfCpltCallback = callback;
					break;
				case BSP_UART_RX_CPLT_CB:
					bsp_uart_callback.xxx.RxCpltCallback = callback;
					break;
				case BSP_UART_ERROR_CB:
					bsp_uart_callback.xxx.ErrorCallback = callback;
					break;
				case BSP_UART_ABORT_CPLT_CB:
					bsp_uart_callback.xxx.AbortCpltCallback = callback;
					break;
				case BSP_UART_ABORT_TX_CPLT_CB:
					bsp_uart_callback.xxx.AbortTxCpltCallback = callback;
					break;
				case BSP_UART_ABORT_RX_CPLT_CB:
					bsp_uart_callback.xxx.AbortRxCpltCallback = callback;
					break;
				case BSP_UART_IDLE_LINE_CB:
					bsp_uart_callback.xxx.IdleLineCallback = callback;
				default:
					return -1;
			}
			break;
		*/
	}
	return 0;
}
