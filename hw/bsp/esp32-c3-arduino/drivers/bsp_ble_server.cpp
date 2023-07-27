#include "bsp_ble_server.h"

#include "Arduino.h"
#include "BleSerial.h"
static BleSerial bleSerial;

void bsp_ble_server_init(const char *name) { bleSerial.begin(name); }

uint32_t bsp_ble_server_avaliable() { return bleSerial.available(); }

bsp_status_t bsp_ble_server_transmit(const uint8_t *data, size_t size) {
  bleSerial.write(data, size);
  return BSP_OK;
}

uint32_t bsp_ble_server_receive(uint8_t *data, size_t size) {
  return bleSerial.readBytes(data, size);
}
