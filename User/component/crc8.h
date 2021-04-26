/*
  参考了Linux
*/

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CRC8_INIT 0xFF

uint8_t CRC8_Calc(const uint8_t *buf, size_t len, uint8_t crc);
bool CRC8_Verify(const uint8_t *buf, size_t len);
