#include "bsp_usb.h"

#include "bsp_def.h"
#include "bsp_uart.h"
#include "main.h"

extern uint8_t bsp_usart2_read_byte(void);

bsp_status_t bsp_usb_transmit(const uint8_t *buffer, uint32_t len) {
  bsp_uart_transmit(BSP_UART_MCU, (uint8_t *)(buffer), len, true);
  return BSP_OK;
}

char bsp_usb_read_char(void) { return bsp_usart2_read_byte(); }

size_t bsp_usb_read(uint8_t *buffer, uint32_t len) {
  bsp_uart_receive(BSP_UART_MCU, buffer, len, true);
  return len;
}

bool bsp_usb_connect(void) { return false; }

size_t bsp_usb_avail(void) { return 0; }

void bsp_usb_init() {}

void bsp_usb_update() {}
