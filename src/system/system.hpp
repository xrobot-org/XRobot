#pragma once

namespace System {
template <typename ResultType, typename ArgType, typename... Args>
class TypeErasure {
 public:
  TypeErasure(ResultType (*fun)(ArgType, Args... args), ArgType arg)
      : fun_(fun), arg_(arg) {}

  static ResultType Port(void* addr, Args... args) {
    TypeErasure<ResultType, ArgType, Args...>* self =
        static_cast<TypeErasure<ResultType, ArgType, Args...>*>(addr);
    return self->fun_(self->arg_, args...);
  }

  ResultType (*fun_)(ArgType, Args... args);
  ArgType arg_;
};
}  // namespace System
