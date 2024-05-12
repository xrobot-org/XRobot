#include "bsp.h"

typedef enum {
  BSP_UART_MCU,
  /* BSP_UART_XXX, */
  BSP_UART_NUM,
  BSP_UART_ERR,
} bsp_uart_t;

typedef enum {
  BSP_UART_TX_CPLT_CB,
  BSP_UART_RX_CPLT_CB,
  BSP_UART_IDLE_LINE_CB,
  BSP_UART_CB_NUM,
} bsp_uart_callback_t;

void bsp_uart_init();

bsp_status_t bsp_uart_transmit(bsp_uart_t uart, uint8_t *data, size_t size,
                               bool block);
bsp_status_t bsp_uart_receive(bsp_uart_t uart, uint8_t *buff, size_t size,
                              bool block);

uint32_t bsp_uart_get_count(bsp_uart_t uart);
