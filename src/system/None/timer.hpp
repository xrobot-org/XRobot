#pragma once

#include <list.hpp>
#include <thread.hpp>

#include "system_ext.hpp"

namespace System {
class Timer {
 public:
  typedef struct {
    void* type;
    void (*fun)(void*);
    uint16_t cycle;
    uint16_t count;
    bool running;
  } ControlBlock;

  typedef System::List<ControlBlock>::Node* TimerHandle;

  Timer();

  static bool Refresh(ControlBlock& block, void* arg);

  template <typename FunType, typename ArgType>
  static TimerHandle Create(FunType fun, ArgType arg, uint32_t cycle) {
    (void)static_cast<void (*)(ArgType)>(fun);
    TypeErasure<void, ArgType>* type = static_cast<TypeErasure<void, ArgType>*>(
        malloc(sizeof(TypeErasure<void, ArgType>)));
    *type = TypeErasure<void, ArgType>(fun, arg);
    auto block = new System::List<ControlBlock>::Node;
    block->data_.count = 0;
    block->data_.cycle = cycle;
    block->data_.fun = type->Port;
    block->data_.type = type;
    block->data_.running = true;
    self_->list_.Add(*block);
    return block;
  }

  static void Delete(TimerHandle& handle) {
    self_->list_.Delete(*handle);
    delete (handle);
  }

  static void Start(TimerHandle& handle) { handle->data_.running = true; }

  static void Stop(TimerHandle& handle) { handle->data_.running = false; }

  static void SetCycle(TimerHandle& timer, uint32_t cycle) {
    timer->data_.cycle = cycle;
  }

  static Timer* self_;
  List<ControlBlock> list_;
  Thread thread_;
};
}  // namespace System
