#pragma once

#include "bsp.h"

void bsp_ble_server_init(const char *name);

uint32_t bsp_ble_server_avaliable();

bsp_status_t bsp_ble_server_transmit(const uint8_t *data, size_t size);

uint32_t bsp_ble_server_receive(uint8_t *data, size_t size);
