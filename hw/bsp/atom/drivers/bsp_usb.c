#include "bsp_usb.h"

#include "FreeRTOS.h"
#include "bsp_def.h"
#include "bsp_uart.h"
#include "main.h"
#include "portmacro.h"
#include "semphr.h"

static SemaphoreHandle_t sem;

static uint8_t usb_rx_buff[100] = {0}, usb_buff_len = 0, usb_buff_index = 0;

static volatile bool connected = false, avail = false;

static void uart_tx_cb(void *arg) {
  XB_UNUSED(arg);
  BaseType_t px_higher_priority_task_woken = 0;
  xSemaphoreGiveFromISR(sem, &px_higher_priority_task_woken);
  if (px_higher_priority_task_woken != pdFALSE) {
    portYIELD();
  }
}

bsp_status_t bsp_usb_transmit(const uint8_t *buffer, uint32_t len) {
  xSemaphoreTake(sem, UINT32_MAX);

  bsp_uart_transmit(BSP_UART_MCU, (uint8_t *)(buffer), len, false);

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

size_t bsp_usb_read(uint8_t *buffer, uint32_t len) {
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

size_t bsp_usb_avail(void) { return avail; }

static void uart_rx_cb(void *arg) {
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
  bsp_uart_register_callback(BSP_UART_MCU, BSP_UART_IDLE_LINE_CB, uart_rx_cb,
                             NULL);
  bsp_uart_register_callback(BSP_UART_MCU, BSP_UART_TX_CPLT_CB, uart_tx_cb,
                             NULL);
  bsp_uart_receive(BSP_UART_MCU, usb_rx_buff, sizeof(usb_rx_buff), false);

  sem = xSemaphoreCreateBinary();

  xSemaphoreGive(sem);
}

void bsp_usb_update() {}
