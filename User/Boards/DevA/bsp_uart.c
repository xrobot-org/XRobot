/* Includes ------------------------------------------------------------------*/
#include "bsp_uart.h"
#include "usart.h"


/* Private define ------------------------------------------------------------*/
#define DR16_UART USART1

/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static struct {
    struct {
        void (*TxHalfCpltCallback)(void);        /* UART Tx Half Complete Callback        */
        void (*TxCpltCallback)(void);            /* UART Tx Complete Callback             */
        void (*RxHalfCpltCallback)(void);        /* UART Rx Half Complete Callback        */
        void (*RxCpltCallback)(void);            /* UART Rx Complete Callback             */
        void (*ErrorCallback)(void);             /* UART Error Callback                   */
        void (*AbortCpltCallback)(void);         /* UART Abort Complete Callback          */
        void (*AbortTransmitCpltCallback)(void); /* UART Abort Transmit Complete Callback */
        void (*AbortReceiveCpltCallback)(void);  /* UART Abort Receive Complete Callback  */
        void (*WakeupCallback)(void);            /* UART Wakeup Callback                  */
    } dr16;
	/* Add other uart device here. */
} bsp_uart_callback;

/* Private function  ---------------------------------------------------------*/
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if(huart->Instance == DR16_UART) {
		if (bsp_uart_callback.dr16.TxCpltCallback != NULL) {
			bsp_uart_callback.dr16.TxCpltCallback();
		}
    }
}

void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart) {
    if(huart->Instance == DR16_UART) {
		if (bsp_uart_callback.dr16.TxCpltCallback != NULL) {
			bsp_uart_callback.dr16.TxHalfCpltCallback();
		}
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if(huart->Instance == DR16_UART) {
		if (bsp_uart_callback.dr16.RxCpltCallback != NULL) {
			bsp_uart_callback.dr16.RxCpltCallback();
		}
    }
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
    if(huart->Instance == DR16_UART) {
		if (bsp_uart_callback.dr16.RxHalfCpltCallback != NULL) {
			bsp_uart_callback.dr16.RxHalfCpltCallback();
		}
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if(huart->Instance == DR16_UART) {
		if (bsp_uart_callback.dr16.ErrorCallback != NULL) {
			bsp_uart_callback.dr16.ErrorCallback();
		}
    }
}

void HAL_UART_AbortCpltCallback(UART_HandleTypeDef *huart) {
    if(huart->Instance == DR16_UART) {
		if (bsp_uart_callback.dr16.AbortCpltCallback != NULL) {
			bsp_uart_callback.dr16.AbortCpltCallback();
		}
    }
}

void HAL_UART_AbortTransmitCpltCallback(UART_HandleTypeDef *huart) {
    if(huart->Instance == DR16_UART) {
		if (bsp_uart_callback.dr16.AbortTransmitCpltCallback != NULL) {
			bsp_uart_callback.dr16.AbortTransmitCpltCallback();
		}
    }
}

void HAL_UART_AbortReceiveCpltCallback(UART_HandleTypeDef *huart) {
    if(huart->Instance == DR16_UART) {
		if (bsp_uart_callback.dr16.AbortReceiveCpltCallback != NULL) {
			bsp_uart_callback.dr16.AbortReceiveCpltCallback();
		}
    }
}

/* Exported functions --------------------------------------------------------*/
int BSP_UART_RegisterCallback(BSP_UART_t uart, BSP_UART_Callback_t type, void (*callback)(void)) {
    switch (uart) {
        case BSP_UART_DR16:
            switch (type) {
                case BSP_UART_TX_HALFCOMPLETE_CB:
                    bsp_uart_callback.dr16.TxCpltCallback = callback;
                    break;
                case BSP_UART_TX_COMPLETE_CB:
                    bsp_uart_callback.dr16.TxHalfCpltCallback = callback;
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
                case BSP_UART_WAKEUP_CB:
                    bsp_uart_callback.dr16.WakeupCallback = callback;
                    break;
                default:
                    return -1;
            }
            break;
        default:
            return -1;
    }
    return 0;
}

int BSP_UART_Transmit(BSP_UART_t uart, uint8_t *data, uint16_t len) {
	return 0;
}
int BSP_UART_Receive(BSP_UART_t uart, uint8_t *data, uint16_t len) {
	return 0;
}
