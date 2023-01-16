#include <memory.hpp>

using namespace System;

void* Memory::Malloc(size_t size) { return pvPortMalloc(size); }

void Memory::Free(void* block) { vPortFree(block); }
