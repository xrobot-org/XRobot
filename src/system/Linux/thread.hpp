#pragma once

#include <poll.h>
#include <pthread.h>
#include <stdint.h>

#include <string>

namespace System {
class Thread {
 private:
  template <typename Arg>
  class HelperFunction {
   public:
    HelperFunction(void (*fun)(Arg arg), Arg arg) : fun_(fun), arg_(arg) {}

    static void* Call(void* arg) {
      auto self = static_cast<HelperFunction*>(arg);
      self->fun_(self->arg_);
      return NULL;
    }

    void (*fun_)(Arg arg);
    Arg arg_;
  };

 public:
  typedef enum { IDLE, Low, Medium, High, Realtime } Priority;

  template <typename Fun, typename Arg>
  void Create(Fun fn, Arg arg, const char* name, uint32_t stack_depth,
              Priority priority) {
    (void)name;
    (void)stack_depth;
    (void)priority;

    static_cast<void (*)(Arg)>(fn);
    HelperFunction<Arg>* helper_fn =
        static_cast<HelperFunction<Arg>*>(malloc(sizeof(HelperFunction<Arg>)));

    *helper_fn = HelperFunction<Arg>(fn, arg);

    pthread_create(&this->handle_, NULL, HelperFunction<Arg>::Call, helper_fn);
  }

  static void Sleep(uint32_t microseconds) { poll(NULL, 0, microseconds); }

  void SleepUntil(uint32_t microseconds) { poll(NULL, 0, microseconds); }

  void Stop() { pthread_cancel(this->handle_); }

  static void StartKernel() {
    while (1) poll(NULL, 0, UINT32_MAX);
  }

 private:
  pthread_t handle_;
};
}  // namespace System
