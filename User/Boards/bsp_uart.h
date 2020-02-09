#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum {
    BSP_UART_DR16,
	/* BSP_UART_XXX, */
} BSP_UART_t;

typedef enum {
	BSP_UART_TX_HALFCOMPLETE_CB,           /* UART Tx Half Complete Callback ID        */
	BSP_UART_TX_COMPLETE_CB,               /* UART Tx Complete Callback ID             */
	BSP_UART_RX_HALFCOMPLETE_CB,           /* UART Rx Half Complete Callback ID        */
	BSP_UART_RX_COMPLETE_CB,               /* UART Rx Complete Callback ID             */
	BSP_UART_ERROR_CB,                     /* UART Error Callback ID                   */
	BSP_UART_ABORT_COMPLETE_CB,            /* UART Abort Complete Callback ID          */
	BSP_UART_ABORT_TRANSMIT_COMPLETE_CB,   /* UART Abort Transmit Complete Callback ID */
	BSP_UART_ABORT_RECEIVE_COMPLETE_CB,    /* UART Abort Receive Complete Callback ID  */
	BSP_UART_WAKEUP_CB,                    /* UART Wakeup Callback ID                  */
} BSP_UART_Callback_t;

/* Exported functions prototypes ---------------------------------------------*/
int BSP_UART_RegisterCallback(BSP_UART_t uart, BSP_UART_Callback_t type, void (*callback)(void));

int BSP_UART_Transmit(BSP_UART_t uart, uint8_t *data, uint16_t len);
int BSP_UART_Receive(BSP_UART_t uart, uint8_t *data, uint16_t len);
