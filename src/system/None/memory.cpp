#include <cstdlib>
#include <memory.hpp>

using namespace System;

void* operator new(std::size_t size) { return malloc(size); }

void operator delete(void* ptr) noexcept { free(ptr); }

void* Memory::Malloc(size_t size) { return malloc(size); }

void Memory::Free(void* block) { free(block); }
