#include "bsp_flash.h"

#include "mf.h"

bsp_status_t bsp_flash_init() {
  mf_init();
  return BSP_OK;
}

size_t bsp_flash_check_blog(const char *name) {
  mf_key_info_t *key = mf_search_key(name);

  if (key) {
    return key->data_size;
  } else {
    return 0;
  }
}

void bsp_flash_get_blog(const char *name, uint8_t *buff, uint32_t len) {
  mf_key_info_t *key = mf_search_key(name);

  if (key == NULL) {
    return;
  }

  if (key->data_size != len) {
    return;
  }

  memcpy(buff, mf_get_key_data(key), len);
}

void bsp_flash_set_blog(const char *name, const uint8_t *buff, uint32_t len) {
  mf_key_info_t *key = mf_search_key(name);

  if (key != NULL && key->data_size != len) {
    return;
  }

  mf_add_key(name, (uint8_t *)buff, len);
  mf_save();
}
