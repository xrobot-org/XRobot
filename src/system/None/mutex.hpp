#pragma once

#include <cstdint>
#include <cstdio>
#include <thread.hpp>

#include "bsp_time.h"
#include "om.hpp"

namespace System {
class Mutex {
 public:
  Mutex(bool unlock = true) {
    om_mutex_init(&this->handle_);
    if (!unlock) {
      om_mutex_lock(&this->handle_);
    }
  }

  void Unlock() { om_mutex_unlock(&this->handle_); }

  bool Lock() { return om_mutex_lock(&this->handle_) == OM_OK; }

 private:
  om_mutex_t handle_;
};
}  // namespace System
