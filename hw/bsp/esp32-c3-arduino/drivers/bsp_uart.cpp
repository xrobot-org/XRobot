#include "bsp_uart.h"

#include "Arduino.h"
#include "HardwareSerial.h"

void bsp_uart_init() { Serial1.begin(115200, SERIAL_8N1, 2, 4); }

int8_t bsp_uart_transmit(bsp_uart_t uart, uint8_t *data, size_t size,
                         bool block) {
  (void)uart;
  (void)block;
  Serial1.write(data, size);
  return BSP_OK;
}

int8_t bsp_uart_receive(bsp_uart_t uart, uint8_t *buff, size_t size,
                        bool block) {
  (void)uart;
  (void)block;
  Serial1.readBytes(buff, size);
  return BSP_OK;
}
