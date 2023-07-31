#include "bsp_usb.h"

#include <stdio.h>

#include "Arduino.h"
#include "FreeRTOS.h"
#include "HWCDC.h"
#include "task.h"

static bool connected = false;

bsp_status_t bsp_usb_transmit(const uint8_t *buffer, uint32_t len) {
  Serial.write(buffer, len);
  return BSP_OK;
}

char bsp_usb_read_char(void) { return Serial.read(); }

bool bsp_usb_connect(void) {
  if (!connected) {
    connected = Serial.available() != 0;
  }

  return connected;
}

size_t bsp_usb_avail(void) { return Serial.available(); }

void bsp_usb_init() { Serial.begin(); }

void bsp_usb_update() { vTaskSuspend(xTaskGetCurrentTaskHandle()); }
