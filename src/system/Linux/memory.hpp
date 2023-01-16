#pragma once

#include <malloc.h>

#include <cstdint>

namespace System {
class Memory {
 public:
  static void* Malloc(size_t size);
  static void Free(void* block);
};
}  // namespace System
