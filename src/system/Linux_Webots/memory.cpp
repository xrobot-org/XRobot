#include "memory.hpp"

using namespace System;

void* Memory::Malloc(size_t size) { return malloc(size); }

void Memory::Free(void* block) { free(block); }
