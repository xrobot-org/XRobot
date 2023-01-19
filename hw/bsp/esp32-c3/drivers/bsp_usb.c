#include "bsp_usb.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

static bool connected = false;

int8_t bsp_usb_transmit(const uint8_t *buffer, uint32_t len) {
  for (int i = 0; i < len; i++) {
    putchar(buffer[i]);
  }
  printf("\033[s\033[1A\n\033[u");
  vTaskDelay(1);
  return BSP_OK;
}

char bsp_usb_read_char(void) {
  int buff = 0;
  while ((buff = getchar()) == EOF) {
    vTaskDelay(1);
  }

  return buff;
}

bool bsp_usb_connect(void) {
  if (!connected) {
    connected = getchar() != EOF;
  }

  return connected;
}

uint32_t bsp_usb_avail(void) { return true; }

void bsp_usb_init() {
  printf("\n\033[2K\rPress enter to open terminal.\033[s\033[1A\n\033[u");
}

void bsp_usb_update() { vTaskSuspend(xTaskGetCurrentTaskHandle()); }
