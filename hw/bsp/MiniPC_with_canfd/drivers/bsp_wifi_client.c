#include "bsp_wifi_client.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "bsp.h"
#include "bsp_def.h"
#include "bsp_linux_base.h"

static char cmd_buff[100];

void bsp_wifi_client_init() {}

bsp_status_t bsp_wifi_connect(const char *name, const char *password) {
  (void)snprintf(cmd_buff, sizeof(cmd_buff), "nmcli con delete %s", name);
  (void)system(cmd_buff);
  (void)snprintf(cmd_buff, sizeof(cmd_buff),
                 "nmcli dev wifi connect %s password \"%s\" name %s", name,
                 password, name);
  if (bsp_shell_success(system(cmd_buff))) {
    return BSP_OK;
  } else {
    return BSP_ERR;
  }
}

bsp_status_t bsp_wifi_delete(const char *name) {
  (void)snprintf(cmd_buff, sizeof(cmd_buff), "nmcli con del %s", name);
  if (bsp_shell_success(system(cmd_buff))) {
    return BSP_OK;
  } else {
    return BSP_ERR;
  }
}

bool bsp_wifi_connected() {
  int ret = open("/sys/class/net/wlp0s20f3/operstate", O_RDONLY);

  char status[3] = "wl\0";
  (void)read(ret, status, 2);
  status[2] = '\0';
  if (0 == strcmp("up", status)) {
    return true;
  } else {
    return false;
  }
}
