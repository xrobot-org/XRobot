#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"

bsp_status_t bsp_flash_init();

size_t bsp_flash_check_blog(const char* name);

void bsp_flash_get_blog(const char* name, uint8_t* buff, uint32_t len);

void bsp_flash_set_blog(const char* name, const uint8_t* buff, uint32_t len);

#ifdef __cplusplus
}
#endif
