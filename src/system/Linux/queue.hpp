#pragma once

#include <mutex.hpp>

#include "bsp_time.h"
#include "om.hpp"

namespace System {
template <typename Data>
class Queue {
 public:
  Queue(uint16_t length) {
    om_fifo_create(&fifo_, malloc(length * sizeof(Data)), length, sizeof(Data));
  }

  bool Send(const Data& data) {
    mutex_.Lock();
    if (om_fifo_write(&fifo_, &data) == OM_OK) {
      mutex_.Unlock();
      return true;
    }
    mutex_.Unlock();
    return false;
  }

  bool Receive(Data& data) {
    mutex_.Lock();
    if (om_fifo_read(&fifo_, &data) == OM_OK) {
      mutex_.Unlock();
      return true;
    } else {
      mutex_.Unlock();
      return false;
    }
  }

  bool Overwrite(const Data& data) {
    mutex_.Lock();
    bool ans = om_fifo_overwrite(&fifo_, &data) == OM_OK;
    mutex_.Unlock();
    return ans;
  }

  bool Reset() {
    mutex_.Lock();
    bool ans = om_fifo_reset(&fifo_) == OM_OK;
    mutex_.Unlock();

    return ans;
  }

  uint32_t Size() {
    mutex_.Lock();
    return om_fifo_readable_item_count(&fifo_);
    mutex_.Unlock();
  }

 private:
  om_fifo_t fifo_;
  System::Mutex mutex_;
};
}  // namespace System
