#include "bsp_wifi_client.h"

#include <stdio.h>

static char cmd_buff[100];

void bsp_wifi_client_init() {}

int8_t bsp_wifi_connect(const char* name, const char* password) {
  (void)name;
  (void)password;
  (void)snprintf(cmd_buff, sizeof(cmd_buff),
                 "nmcli dev wifi connect %s password \"%s\"", name, password);
  system(cmd_buff);
  return BSP_OK;
}

bool bsp_wifi_connected() {
  return system("ping -c 1 xrobot-org.github.io > /dev/null") == 0;
}
