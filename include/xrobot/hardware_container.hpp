#pragma once
#include <tuple>
#include <cstring>
#include <cstdlib>
#include <iostream>

namespace xrobot {

template <typename T> struct Entry {
  T &object;
  const char *name;
};

template <typename... Entries> class HardwareContainer {
public:
  constexpr HardwareContainer(Entries... entries) : entries_{entries...} {}

  template <typename T> T *Find(const char *name) const {
    T *result = nullptr;
    std::apply(
        [&](const auto &...e) {
          ((strcmp(e.name, name) == 0 ? result = &e.object : void()), ...);
        },
        entries_);
    return result;
  }

  template <typename T> T *FindOrAbort(const char *name) const {
    T *ptr = Find<T>(name);
    if (!ptr) {
      std::cerr << "[ERROR] Peripheral not found: " << name << std::endl;
      std::abort();
    }
    return ptr;
  }

  template<typename F>
  void RegisterAll(F &&func) const {
      std::apply([&](const auto &... e) {
          (func(e), ...);
      }, entries_);
  }

private:
  std::tuple<Entries...> entries_;
};

template <typename T> constexpr auto MakeEntry(T &obj, const char *name) {
  return Entry<T>{obj, name};
}

} // namespace xrobot