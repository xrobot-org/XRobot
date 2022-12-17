#pragma once

#include <poll.h>
#include <stdint.h>
#include <stdio.h>

#include <queue>
#include <thread.hpp>

namespace System {
template <typename Data>
class Queue {
 public:
  Queue(uint16_t length) : max_length_(length) {}

  bool Send(const Data& data, uint32_t timeout) {
    while (this->handle_.size() >= this->max_length_ && timeout) {
      System::Thread::Sleep(1);
      timeout--;
    }

    if (this->handle_.size() < this->max_length_) {
      this->handle_.push(data);
      return true;
    } else {
      return false;
    }
  }

  bool Receive(Data& data, uint32_t timeout) {
    while (this->handle_.size() >= 0 && timeout) {
      System::Thread::Sleep(1);
      timeout--;
    }

    if (this->handle_.size() > 0) {
      data = this->handle_.front();
      return true;
    } else {
      return false;
    }
  }

  bool Overwrite(const Data& data) {
    Reset();
    this->handle_.push(data);
    return true;
  }

  bool SendFromISR(const Data& data) { return Send(data); }

  bool ReceiveFromISR(Data& data) { return Receive(data); }

  bool OverwriteFromISR(const Data& data) { return Overwrite(data); }

  bool Reset() {
    while (this->handle_.size()) this->handle_.pop();
    return true;
  }

 private:
  std::queue<Data> handle_;
  size_t max_length_;
};
}  // namespace System
