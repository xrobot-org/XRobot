#include "bsp_usb.h"

#include "bsp_uart.h"
#include "main.h"

static uint8_t usb_rx_buff[100] = {0}, usb_buff_len = 0, usb_buff_index = 0;

static volatile bool connected = false, avail = false;

int8_t bsp_usb_transmit(const uint8_t *buffer, uint32_t len) {
  while (len >= 64) {
    bsp_uart_transmit(BSP_UART_MCU, (uint8_t *)(buffer), 64, true);
    buffer += 64;
    len -= 64;
  }

  bsp_uart_transmit(BSP_UART_MCU, (uint8_t *)(buffer), len, true);

  return BSP_OK;
}

char bsp_usb_read_char(void) {
  if (avail) {
    char data = usb_rx_buff[usb_buff_index++];
    if (usb_buff_index >= usb_buff_len) {
      avail = false;
    }
    return data;
  } else {
    return 0;
  }
}

uint32_t bsp_usb_read(uint8_t *buffer, uint32_t len) {
  if (avail) {
    uint32_t count = 0;
    while (usb_buff_index < usb_buff_len && count < len) {
      buffer[count++] = usb_rx_buff[usb_buff_index++];
    }

    return count;
  } else {
    return 0;
  }
}

bool bsp_usb_connect(void) { return connected; }

uint32_t bsp_usb_avail(void) { return avail; }

static void usb_rx_cb(void *arg) {
  (void)(arg);
  if (!connected) {
    connected = true;
    bsp_uart_abort_receive(BSP_UART_MCU);
    bsp_uart_receive(BSP_UART_MCU, usb_rx_buff, sizeof(usb_rx_buff), false);
    return;
  }
  usb_buff_len = bsp_uart_get_count(BSP_UART_MCU);
  usb_buff_index = 0;
  avail = true;
  bsp_uart_abort_receive(BSP_UART_MCU);
  bsp_uart_receive(BSP_UART_MCU, usb_rx_buff, sizeof(usb_rx_buff), false);
}

void bsp_usb_init() {
  bsp_uart_register_callback(BSP_UART_MCU, BSP_UART_IDLE_LINE_CB, usb_rx_cb,
                             NULL);
}

void bsp_usb_update() {}
