#pragma once

#include <cstdint>

namespace System {
class Semaphore {
 public:
  Semaphore(bool init_count);
  Semaphore(uint16_t max_count, uint16_t init_count);
  void Give();
  bool Take(uint32_t timeout);
  void GiveFromISR();
  bool TakeFromISR();

 private:
  uint32_t count_;
  uint32_t max_count_;
};
}  // namespace System
