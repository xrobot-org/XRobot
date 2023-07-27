#include "bsp_ble_server.h"

#include "Arduino.h"
#include "BleSerial.h"
static BleSerial bleSerial;

static const char *BLE_SERIAL_SERVICE_UUID =
    "6e400004-b5a3-f393-e0a9-e50e24dcca9e";
static const char *BLE_RX_UUID = "6e400005-b5a3-f393-e0a9-e50e24dcca9e";
static const char *BLE_TX_UUID = "6e400006-b5a3-f393-e0a9-e50e24dcca9e";

void bsp_ble_server_init(const char *name) {
  bleSerial.setUUID(BLE_SERIAL_SERVICE_UUID, BLE_RX_UUID, BLE_TX_UUID);
  bleSerial.begin(name);
  bleSerial.setPIN(666666);
}

uint32_t bsp_ble_server_avaliable() { return bleSerial.available(); }

bsp_status_t bsp_ble_server_transmit(const uint8_t *data, size_t size) {
  bleSerial.write(data, size);
  return BSP_OK;
}

uint32_t bsp_ble_server_receive(uint8_t *data, size_t size) {
  return bleSerial.readBytes(data, size);
}
