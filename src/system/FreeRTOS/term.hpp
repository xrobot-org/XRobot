#pragma once

#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <system_ext.hpp>

#include "ms.h"

namespace System {
class Term {
 public:
  template <typename ArgType>
  class Command {
   public:
    Command(ArgType arg, int (*fun)(ArgType, int, char **), const char *name,
            ms_item_t *dir = ms_get_bin_dir())
        : type_(fun, arg) {
      ms_file_init(&this->cmd_, name, this->Call, NULL, NULL);
      ms_item_add(&this->cmd_, dir);
    }

    static int Call(ms_item_t *cmd, int argc, char **argv) {
      Command<ArgType> *self = ms_container_of(cmd, Command<ArgType>, cmd_);
      return self->type_.Port(&self->type_, argc, argv);
    }

   private:
    ms_item_t cmd_;
    TypeErasure<int, ArgType, int, char **> type_;
  };

  Term();

  static ms_item_t *BinDir() { return ms_get_bin_dir(); }

  static ms_item_t *EtcDir() { return ms_get_etc_dir(); }

  static ms_item_t *DevDir() { return ms_get_dev_dir(); }
};
}  // namespace System
