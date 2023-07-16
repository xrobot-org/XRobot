#pragma once

#include <cstdint>
#include <mutex.hpp>
#include <semaphore.hpp>
#include <thread.hpp>

#include "FreeRTOS.h"
#include "bsp_sys.h"
#include "bsp_time.h"
#include "om.hpp"
#include "projdefs.h"
#include "task.h"

namespace System {
template <typename Data>
class Queue {
 public:
  Queue(uint16_t length) : mutex_(xSemaphoreCreateBinary()) {
    om_fifo_create(&fifo_, malloc(length * sizeof(Data)), length, sizeof(Data));
    xSemaphoreGive(mutex_);
  }

  bool Send(const Data& data) {
    if (bsp_sys_in_isr()) {
      if (xSemaphoreTakeFromISR(mutex_, NULL) != pdTRUE) {
        return false;
      }

      if (om_fifo_write(&fifo_, &data) == OM_OK) {
        xSemaphoreGiveFromISR(mutex_, NULL);
        return true;
      } else {
        xSemaphoreGiveFromISR(mutex_, NULL);
        return false;
      }
    } else {
      xSemaphoreTake(mutex_, UINT32_MAX);
      if (om_fifo_write(&fifo_, &data) == OM_OK) {
        xSemaphoreGive(this->mutex_);
        return true;
      }
      xSemaphoreGive(this->mutex_);
      return false;
    }
  }

  bool Receive(Data& data) {
    if (bsp_sys_in_isr()) {
      if (xSemaphoreTakeFromISR(mutex_, NULL) != pdTRUE) {
        return false;
      }

      if (om_fifo_read(&fifo_, &data) == OM_OK) {
        xSemaphoreGiveFromISR(mutex_, NULL);
        return true;
      }
      xSemaphoreGiveFromISR(mutex_, NULL);
      return false;
    } else {
      xSemaphoreTake(mutex_, UINT32_MAX);
      if (om_fifo_read(&fifo_, &data) == OM_OK) {
        xSemaphoreGive(this->mutex_);
        return true;
      }
      xSemaphoreGive(this->mutex_);
      return false;
    }
  }

  bool Overwrite(const Data& data) {
    if (bsp_sys_in_isr()) {
      if (xSemaphoreTakeFromISR(mutex_, NULL) != pdTRUE) {
        return false;
      }
      if (om_fifo_overwrite(&fifo_, &data) == OM_OK) {
        xSemaphoreGiveFromISR(mutex_, NULL);
        return true;
      }
      xSemaphoreGiveFromISR(mutex_, NULL);
      return false;
    } else {
      xSemaphoreTake(mutex_, UINT32_MAX);

      if (om_fifo_overwrite(&fifo_, &data) == OM_OK) {
        xSemaphoreGive(this->mutex_);
        return true;
      }
      xSemaphoreGive(this->mutex_);
      return false;
    }
  }

  bool Reset() {
    if (bsp_sys_in_isr()) {
      if (xSemaphoreTakeFromISR(mutex_, NULL) != pdTRUE) {
        return false;
      }
      if (om_fifo_reset(&fifo_) == OM_OK) {
        xSemaphoreGiveFromISR(mutex_, NULL);
        return true;
      }
      xSemaphoreGiveFromISR(mutex_, NULL);
      return false;
    } else {
      xSemaphoreTake(mutex_, UINT32_MAX);

      if (om_fifo_reset(&fifo_) == OM_OK) {
        xSemaphoreGive(this->mutex_);
        return true;
      }
      xSemaphoreGive(this->mutex_);
      return false;
    }
  }

  uint32_t Size() {
    uint32_t size = 0;
    if (bsp_sys_in_isr()) {
      if (xSemaphoreTakeFromISR(mutex_, NULL) != pdTRUE) {
        return false;
      }
      size = om_fifo_readable_item_count(&fifo_);
      xSemaphoreGiveFromISR(mutex_, NULL);
      return false;
    } else {
      xSemaphoreTake(mutex_, UINT32_MAX);

      size = om_fifo_readable_item_count(&fifo_);
      xSemaphoreGive(this->mutex_);
    }
    return size;
  }

 private:
  om_fifo_t fifo_;
  SemaphoreHandle_t mutex_;
};
}  // namespace System
