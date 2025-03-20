#pragma once
#include <iostream>
#include <map>
#include <string>

class PeripheralManager {
public:
    static PeripheralManager &Instance() {
        static PeripheralManager instance;
        return instance;
    }

    template<typename T>
    void Register(const char* logic_name, T* obj) {
        peripheral_map_[logic_name] = static_cast<void*>(obj);
    }

    template<typename T>
    T* Find(const char* logic_name) {
        auto it = peripheral_map_.find(logic_name);
        return (it != peripheral_map_.end()) ? static_cast<T*>(it->second) : nullptr;
    }

private:
    std::map<std::string, void*> peripheral_map_;
};