/*
  参考了Linux
*/

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CRC8_INIT 0xFF

uint8_t crc8_calc(const uint8_t *buf, size_t len, uint8_t crc);
bool crc8_verify(const uint8_t *buf, size_t len);
