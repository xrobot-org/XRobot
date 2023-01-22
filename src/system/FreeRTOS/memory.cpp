#include <memory.hpp>

#include "portable.h"

using namespace System;

void* operator new(std::size_t size) { return pvPortMalloc(size); }

void operator delete(void* ptr) noexcept { vPortFree(ptr); }

void* Memory::Malloc(size_t size) { return pvPortMalloc(size); }

void Memory::Free(void* block) { vPortFree(block); }
