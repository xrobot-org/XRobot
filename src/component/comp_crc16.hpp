#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "comp_utils.hpp"

#define CRC16_INIT 0XFFFF

uint16_t crc16_calc(const uint8_t *buf, size_t len, uint16_t crc);
bool crc16_verify(const uint8_t *buf, size_t len);
