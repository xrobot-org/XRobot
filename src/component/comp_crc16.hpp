#pragma once

#include <stddef.h>
#include <stdint.h>

#define CRC16_INIT 0XFFFF

namespace Component {
class CRC16 {
 public:
  static uint16_t Calculate(const uint8_t *buf, size_t len, uint16_t crc);
  static bool Verify(const uint8_t *buf, size_t len);
};
}  // namespace Component
