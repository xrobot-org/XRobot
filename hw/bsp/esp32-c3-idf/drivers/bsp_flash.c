#include "bsp_flash.h"

#include "nvs.h"
#include "nvs_flash.h"

#define STORAGE_NAMESPACE "storage"

bsp_status_t bsp_flash_init() {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  return err == ESP_OK ? BSP_OK : BSP_ERR;
}

size_t bsp_flash_check_blog(const char* name) {
  nvs_handle_t my_handle = 0;
  esp_err_t err = ESP_OK;
  size_t required_size = 0;

  // Open
  err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    return err;
  }

  nvs_get_blob(my_handle, name, NULL, &required_size);

  nvs_close(my_handle);

  return required_size;
}

void bsp_flash_get_blog(const char* name, uint8_t* buff, uint32_t len) {
  nvs_handle_t my_handle = 0;
  esp_err_t err = ESP_OK;

  if (bsp_flash_check_blog(name) != len) {
    return;
  }

  // Open
  err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    return;
  }

  nvs_get_blob(my_handle, name, buff, (size_t*)(&len));

  nvs_close(my_handle);
}

void bsp_flash_set_blog(const char* name, const uint8_t* buff, uint32_t len) {
  nvs_handle_t my_handle = 0;
  esp_err_t err = ESP_OK;
  size_t required_size = 0;

  // Open
  err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    return;
  }

  nvs_set_blob(my_handle, name, buff, len);

  err = nvs_commit(my_handle);

  nvs_close(my_handle);
}
