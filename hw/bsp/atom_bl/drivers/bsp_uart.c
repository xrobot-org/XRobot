#include "bsp_uart.h"

#include "bsp_def.h"
#include "main.h"
#include "stm32g4xx_it.h"

uint8_t bsp_usart2_read_byte(void) {
  while (!LL_USART_IsActiveFlag_RXNE_RXFNE(USART2)) {
  }

  return LL_USART_ReceiveData8(USART2);
}

void bsp_usart2_send_byte(uint8_t Byte) {
  LL_USART_TransmitData8(USART2, (Byte & 0xFF));

  while (!LL_USART_IsActiveFlag_TC(USART2)) {
  }
}

bsp_status_t bsp_uart_transmit(bsp_uart_t uart, uint8_t *data, size_t size,
                               bool block) {
  XB_UNUSED(uart);

  if (block) {
    while (size--) {
      bsp_usart2_send_byte(*(data++));
    }
  } else {
    XB_ASSERT(false);
  }

  return BSP_OK;
}

bsp_status_t bsp_uart_receive(bsp_uart_t uart, uint8_t *buff, size_t size,
                              bool block) {
  XB_UNUSED(uart);

  if (block) {
    while (size--) {
      *(buff++) = bsp_usart2_read_byte();
    }
  } else {
    XB_ASSERT(false);
  }

  return BSP_OK;
}

uint32_t bsp_uart_get_count(bsp_uart_t uart) {
  XB_UNUSED(uart);
  return 0;
}

bsp_status_t bsp_uart_abort_receive(bsp_uart_t uart) {
  XB_UNUSED(uart);
  return BSP_OK;
}
