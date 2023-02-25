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
  } ControlBlock;

  Timer();

  static bool Refresh(ControlBlock& block, void* arg);

  template <typename FunType, typename ArgType>
  static void Create(FunType fun, ArgType arg, uint32_t cycle) {
    (void)static_cast<void (*)(ArgType)>(fun);
    TypeErasure<void, ArgType>* type = static_cast<TypeErasure<void, ArgType>*>(
        malloc(sizeof(TypeErasure<void, ArgType>)));
    *type = TypeErasure<void, ArgType>(fun, arg);
    ControlBlock block;
    block.count = 0;
    block.cycle = cycle;
    block.fun = type->Port;
    block.type = type;
    self_->list_.Add(block);
  }

  static Timer* self_;
  List<ControlBlock> list_;
  Thread thread_;
};
}  // namespace System
