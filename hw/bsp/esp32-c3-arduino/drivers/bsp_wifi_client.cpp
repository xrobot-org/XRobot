#include "bsp_wifi_client.h"

#include "Arduino.h"
#include "WiFi.h"
#include "WiFiClient.h"

static WiFiClient* wifi;

void bsp_wifi_client_init() { wifi = new WiFiClient; }

int8_t bsp_wifi_connect(const char* name, const char* password) {
  WiFi.begin(name, password);
  return BSP_OK;
}

bool bsp_wifi_connected() { return WiFi.status() == WL_CONNECTED; }
