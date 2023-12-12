#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp_def.h"

void bsp_wifi_client_init();

bsp_status_t bsp_wifi_connect(const char* name, const char* password);

bsp_status_t bsp_wifi_delete(const char* name);

bool bsp_wifi_connected();

#ifdef __cplusplus
}
#endif
