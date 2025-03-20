#pragma once
#include "peripheral_manager.hpp"
#include <utility>
#include <tuple>
#include <cstring>

namespace xrobot {

class PlatformAdapter {
public:
    template<typename Container>
    static void Init(const Container &container) {
        container.RegisterAll([](const auto &entry) {
            PeripheralManager::Instance().Register(entry.name, &entry.object);
        });
    }
};

} // namespace xrobot