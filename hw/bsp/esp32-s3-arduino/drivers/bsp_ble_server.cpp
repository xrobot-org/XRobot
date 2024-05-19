#include "bsp_ble_server.h"

#include <stdint.h>

#include "Arduino.h"
#include "BLEDevice.h"
#include "om.h"
#include "om_fifo.h"

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define WIFI_CONFIG_UUID "5fafc202-2fb5-469e-8fcc-c5c9c332914b"

static om_fifo_t rx_fifo;
static uint8_t buff[100];
static BLECharacteristic *p_wifi_config_characteristic;

class WifiConfigCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    om_fifo_writes(&rx_fifo, pCharacteristic->getData(),
                   pCharacteristic->getLength());
  }
};

void bsp_ble_server_init(const char *name) {
  om_fifo_create(&rx_fifo, buff, sizeof(buff), sizeof(uint8_t));
  BLEDevice::init(name);
  BLEServer *p_server = BLEDevice::createServer();
  BLEService *p_service = p_server->createService(SERVICE_UUID);
  p_wifi_config_characteristic = p_service->createCharacteristic(
      WIFI_CONFIG_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  p_wifi_config_characteristic->setCallbacks(new WifiConfigCallback());

  p_service->start();
  BLEAdvertising *p_advertising = BLEDevice::getAdvertising();
  p_advertising->addServiceUUID(SERVICE_UUID);
  p_advertising->setScanResponse(true);
  p_advertising->setMinPreferred(0x06);
  p_advertising->setMinPreferred(0x12);

  BLEDevice::startAdvertising();
}

uint32_t bsp_ble_server_avaliable() {
  return om_fifo_readable_item_count(&rx_fifo);
}

bsp_status_t bsp_ble_server_transmit(const uint8_t *data, size_t size) {
  memcpy(buff, data, size);
  p_wifi_config_characteristic->setValue(buff, size);
  return BSP_OK;
}

uint32_t bsp_ble_server_receive(uint8_t *data, size_t size) {
  return om_fifo_reads(&rx_fifo, data, size) == OM_OK ? 0 : size;
}
