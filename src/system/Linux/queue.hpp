#pragma once

#include <mutex.hpp>
#include <semaphore.hpp>
#include <thread.hpp>

#include "bsp_time.h"
#include "om.hpp"

namespace System {
template <typename Data>
class Queue {
 public:
  Queue(uint16_t length) {
    om_fifo_create(&fifo_, malloc(length * sizeof(Data)), length, sizeof(Data));
  }

  bool Send(const Data& data, uint32_t timeout) {
    uint32_t start_time = bsp_time_get_ms();
    if (!mutex_.Lock(timeout)) {
      return false;
    }

    if (om_fifo_write(&fifo_, &data) == OM_OK) {
      mutex_.Unlock();
      return true;
    }

    bool ans = false;
    while (bsp_time_get_ms() - start_time < timeout) {
      ans = om_fifo_write(&fifo_, &data) == OM_OK;
      if (ans) {
        break;
      }
      mutex_.Unlock();
      System::Thread::Sleep(1);
      while (bsp_time_get_ms() - start_time < timeout) {
        if (mutex_.Lock(1)) {
          break;
        }
      }
    }

    mutex_.Unlock();

    return ans;
  }

  bool Receive(Data& data, uint32_t timeout) {
    uint32_t start_time = bsp_time_get_ms();
    if (!mutex_.Lock(timeout)) {
      return false;
    }

    if (om_fifo_read(&fifo_, &data) == OM_OK) {
      mutex_.Unlock();
      return true;
    }

    bool ans = false;
    while (bsp_time_get_ms() - start_time < timeout) {
      ans = om_fifo_read(&fifo_, &data) == OM_OK;
      if (ans) {
        break;
      }
      mutex_.Unlock();
      System::Thread::Sleep(1);
      while (1) {
        if (mutex_.Lock(1)) {
          break;
        } else {
          if (bsp_time_get_ms() - start_time < timeout) {
            return false;
          }
        }
      }
    }

    mutex_.Unlock();

    return ans;
  }

  bool Overwrite(const Data& data) {
    if (!mutex_.Lock(0)) {
      return false;
    }

    bool ans = om_fifo_overwrite(&fifo_, &data) == OM_OK;

    mutex_.Unlock();

    return ans;
  }

  bool SendFromISR(const Data& data) {
    if (!mutex_.LockFromISR()) {
      return false;
    }

    bool ans = om_fifo_write(&fifo_, &data) == OM_OK;

    mutex_.UnlockFromISR();

    return ans;
  }

  bool ReceiveFromISR(Data& data) {
    if (!mutex_.LockFromISR()) {
      return false;
    }

    bool ans = om_fifo_read(&fifo_, &data) == OM_OK;

    mutex_.UnlockFromISR();

    return ans;
  }

  bool OverwriteFromISR(const Data& data) {
    if (!mutex_.LockFromISR()) {
      return false;
    }

    bool ans = om_fifo_overwrite(&fifo_, &data) == OM_OK;

    mutex_.UnlockFromISR();

    return ans;
  }

  bool Reset() {
    if (!mutex_.Lock(0)) {
      return false;
    }

    bool ans = om_fifo_reset(&fifo_) == OM_OK;

    mutex_.Unlock();

    return ans;
  }

  uint32_t Size() { return om_fifo_readable_item_count(&fifo_); }

 private:
  om_fifo_t fifo_;
  System::Mutex mutex_;
};
}  // namespace System
