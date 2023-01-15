/*
  参考了Linux
*/

#pragma once

#include "component.hpp"

#define CRC8_INIT 0xFF

namespace Component {
class CRC8 {
 public:
  static uint8_t Calculate(const uint8_t *buf, size_t len, uint8_t crc);
  static bool Verify(const uint8_t *buf, size_t len);
};
}  // namespace Component
