#pragma once
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <tuple>

namespace XRobot {

template <typename T> struct Entry {
  T &object;
  const char *name;
};

template <typename... Entries> class HardwareContainer {
public:
  constexpr HardwareContainer(Entries... entries) : entries_{entries...} {}

  template <typename F> void RegisterAll(F &&func) const {
    std::apply([&](const auto &...e) { (func(e), ...); }, entries_);
  }

private:
  std::tuple<Entries...> entries_;
};

template <typename T> constexpr auto MakeEntry(T &obj, const char *name) {
  return Entry<T>{obj, name};
}

} // namespace XRobot