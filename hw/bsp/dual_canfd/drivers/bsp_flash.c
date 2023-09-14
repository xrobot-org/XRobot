#include "bsp_flash.h"

#include "bsp_def.h"

bsp_status_t bsp_flash_init() { return BSP_OK; }

size_t bsp_flash_check_blog(const char* name) {
  XB_UNUSED(name);

  return 0;
}

void bsp_flash_get_blog(const char* name, uint8_t* buff, uint32_t len) {
  XB_UNUSED(name);
  XB_UNUSED(buff);
  XB_UNUSED(len);
}

void bsp_flash_set_blog(const char* name, const uint8_t* buff, uint32_t len) {
  XB_UNUSED(name);
  XB_UNUSED(buff);
  XB_UNUSED(len);
}
