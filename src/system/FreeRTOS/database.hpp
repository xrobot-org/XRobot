#include "easyflash.h"

namespace System {
class Database {
 public:
  Database();

  static uint32_t Find(const char* name) {
    size_t len;
    ef_get_env_blob(name, NULL, 0, &len);
    return len;
  }

  static uint32_t Get(const char* name, void* buff, uint32_t len) {
    return ef_get_env_blob(name, buff, len, NULL);
  }

  static bool Set(const char* name, void* data, uint32_t len) {
    uint32_t write_times;
    ef_get_env_blob("WRITE TIMES", &write_times, sizeof(uint32_t), NULL);

    write_times += 2;

    ef_set_env_blob("WRITE TIMES", &write_times, sizeof(uint32_t));

    return ef_set_env_blob(name, data, len) == EF_NO_ERR;
  }
};
}  // namespace System
