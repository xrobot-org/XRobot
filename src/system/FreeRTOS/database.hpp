#pragma once

#include <cstring>

#include "bsp_flash.h"

namespace System {
class Database {
 public:
  Database();
  template <typename Data>
  class Key {
   public:
    Key(const char* name) : name_(name) {
      if (bsp_flash_check_blog(name) == sizeof(Data)) {
        bsp_flash_get_blog(name, reinterpret_cast<uint8_t*>(&this->data_),
                           sizeof(Data));
      } else {
        memset(&this->data_, 0, sizeof(Data));
        bsp_flash_set_blog(name, reinterpret_cast<uint8_t*>(&this->data_),
                           sizeof(Data));
      }
    }

    void Set() {
      bsp_flash_set_blog(name_, reinterpret_cast<uint8_t*>(&this->data_),
                         sizeof(Data));
    }

    void Get() {
      bsp_flash_get_blog(name_, reinterpret_cast<uint8_t*>(&this->data_),
                         sizeof(Data));
    }

    Data data_;
    const char* name_;
  };
};
}  // namespace System
