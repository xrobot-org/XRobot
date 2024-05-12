#include "bsp_wifi_client.h"

#include <cstring>

#include "Arduino.h"
#include "WiFi.h"
#include "WiFiClient.h"

static WiFiClient* wifi;

void bsp_wifi_client_init() { wifi = new WiFiClient; }

bsp_status_t bsp_wifi_connect(const char* name, const char* password) {
  WiFi.begin(name, password);
  return BSP_OK;
}

bsp_status_t bsp_wifi_delete(const char* name) {
  if (strcmp(name, WiFi.SSID().c_str()) == 0) {
    WiFi.disconnect();
  }
  return BSP_OK;
}

bool bsp_wifi_connected() { return WiFi.isConnected(); }
