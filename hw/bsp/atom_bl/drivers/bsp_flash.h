#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"

#define BSP_FLASH_BLOCK_SIZE (2048)
#define BSP_FLASH_APP_ADDR (0x8005000)
#define BSP_FLASH_APP_SIZE (1024 * 104)

bsp_status_t bsp_flash_init();

bsp_status_t bsp_flash_wirte(void* addr, size_t size, const void* buff);

bsp_status_t bsp_flash_erase(void* addr, size_t size);

#ifdef __cplusplus
}
#endif
