#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

namespace System {
class Database {
 public:
  Database();

  static uint32_t Find(const char* name) {
    if (access((path_ + name).c_str(), W_OK)) {
      return 0;
    } else {
      return 1;
    }
  }

  static uint32_t Get(const char* name, void* buff, uint32_t len) {
    FILE* fd;

    if (!access((path_ + name).c_str(), W_OK)) {
      fd = fopen((path_ + name).c_str(), "r");
    } else {
      fd = fopen((path_ + name).c_str(), "w+");
    }

    fread(buff, len, 1, fd);

    fclose(fd);

    return len;
  }

  static bool Set(const char* name, void* data, uint32_t len) {
    FILE* fd = fopen((path_ + name).c_str(), "w+");

    fwrite(data, len, 1, fd);

    fclose(fd);

    return true;
  }

  static std::string path_;
};
}  // namespace System
