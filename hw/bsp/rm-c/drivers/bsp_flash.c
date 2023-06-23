#include "bsp_flash.h"

#include "easyflash.h"

bsp_status_t bsp_flash_init() { return easyflash_init() != EF_NO_ERR; }

size_t bsp_flash_check_blog(const char* name) {
  size_t len = 0;
  ef_get_env_blob(name, NULL, 0, &len);

  return len;
}

void bsp_flash_get_blog(const char* name, uint8_t* buff, uint32_t len) {
  ef_get_env_blob(name, buff, len, NULL);
}

void bsp_flash_set_blog(const char* name, const uint8_t* buff, uint32_t len) {
  ef_set_env_blob(name, buff, len);
}
