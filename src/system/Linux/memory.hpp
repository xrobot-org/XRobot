#pragma once

#include <malloc.h>

#include <cstdint>

namespace System {
class Memory {
 public:
  static void* Malloc(size_t size) { return malloc(size); }
  static void Free(void* block) { free(block); }
};
}  // namespace System
