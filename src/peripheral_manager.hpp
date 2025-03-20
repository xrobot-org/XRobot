#pragma once
#include "libxr_def.hpp"
#include "list.hpp"
#include <cstring>

struct PeripheralEntry {
  const char *name;
  void *object;
};

class PeripheralManager {
public:
  LibXR::List peripheral_list_;

  template <typename Container> PeripheralManager(const Container &container) {
    container.RegisterAll([&](const auto &entry) {
      Register(entry.name, &entry.object);
    });
  }

  template <typename T> void Register(const char *logic_name, T *obj) {
    auto node = new LibXR::List::Node<PeripheralEntry>(
        {logic_name, static_cast<void *>(obj)});
    peripheral_list_.Add(*node);
  }

  template <typename T> T *Find(const char *logic_name) {
    T *result = nullptr;
    peripheral_list_.Foreach<PeripheralEntry>([&](PeripheralEntry &entry) {
      if (strcmp(entry.name, logic_name) == 0) {
        result = static_cast<T *>(entry.object);
        return ErrorCode::FAILED;
      }
      return ErrorCode::OK;
    });
    return result;
  }

  template <typename T> T *FindOrExit(const char *logic_name) {
    auto ptr = Find<T>(logic_name);
    ASSERT(ptr != nullptr);
    return ptr;
  }
};