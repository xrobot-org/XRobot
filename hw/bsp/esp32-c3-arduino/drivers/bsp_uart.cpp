#include "bsp_uart.h"

#include "Arduino.h"
#include "HardwareSerial.h"

static size_t uart_count;

void bsp_uart_init() { Serial1.begin(115200, SERIAL_8N1, 7, 6); }

bsp_status_t bsp_uart_transmit(bsp_uart_t uart, uint8_t *data, size_t size,
                               bool block) {
  (void)uart;
  (void)block;
  Serial1.write(data, size);
  return BSP_OK;
}

bsp_status_t bsp_uart_receive(bsp_uart_t uart, uint8_t *buff, size_t size,
                              bool block) {
  (void)uart;
  (void)block;
  uart_count = Serial1.readBytes(buff, size);
  return BSP_OK;
}

uint32_t bsp_uart_get_count(bsp_uart_t uart) {
  (void)uart;
  return uart_count;
}
