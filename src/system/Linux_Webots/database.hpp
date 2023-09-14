#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace System {
class Database {
 public:
  Database();

  template <typename Data>
  class Key {
   public:
    Key(const char* name) : name_(name) {
      FILE* fd = NULL;
      if (!access((path_ + name).c_str(), W_OK)) {
        fd = fopen((path_ + name).c_str(), "r");
        static_cast<void>(fread(&this->data_, sizeof(Data), 1, fd));
        static_cast<void>(fclose(fd));
      } else {
        fd = fopen((path_ + name).c_str(), "w+");
        memset(&this->data_, 0, sizeof(Data));
        static_cast<void>(fwrite(&this->data_, sizeof(Data), 1, fd));
        static_cast<void>(fclose(fd));
      }
    }

    Key(const char* name, const Data& init_value) : name_(name) {
      FILE* fd = NULL;
      if (!access((path_ + name).c_str(), W_OK)) {
        fd = fopen((path_ + name).c_str(), "r");
        static_cast<void>(fread(&this->data_, sizeof(Data), 1, fd));
        static_cast<void>(fclose(fd));
      } else {
        fd = fopen((path_ + name).c_str(), "w+");
        this->data_ = init_value;
        static_cast<void>(fwrite(&this->data_, sizeof(Data), 1, fd));
        static_cast<void>(fclose(fd));
      }
    }

    void Set() {
      FILE* fd = NULL;
      fd = fopen((path_ + name_).c_str(), "w+");
      static_cast<void>(fwrite(&this->data_, sizeof(Data), 1, fd));
      static_cast<void>(fclose(fd));
    }

    void Set(const Data& data) {
      this->data_ = data;
      FILE* fd = NULL;
      fd = fopen((path_ + name_).c_str(), "w+");
      static_cast<void>(fwrite(&this->data_, sizeof(Data), 1, fd));
      static_cast<void>(fclose(fd));
    }

    void Get() {
      FILE* fd = NULL;
      fd = fopen((path_ + name_).c_str(), "r");
      static_cast<void>(fread(&this->data_, sizeof(Data), 1, fd));
      static_cast<void>(fclose(fd));
    }

    operator Data() { return data_; }

    Data data_;
    const char* name_;
  };

  static std::string path_;
};
}  // namespace System
