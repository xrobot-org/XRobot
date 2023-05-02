#pragma once

#include "bsp.h"

void bsp_wifi_client_init();

int8_t bsp_wifi_connect(const char* name, const char* password);

bool bsp_wifi_connected();
