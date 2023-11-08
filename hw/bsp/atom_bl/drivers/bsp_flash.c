#include "bsp_flash.h"

#include "bsp_def.h"
#include "stm32g4xx_hal.h"

bsp_status_t bsp_flash_init();

bsp_status_t bsp_flash_wirte(void* addr, size_t size, const void* buff) {
  void* startaddr = addr;
  const uint8_t* data = buff;

  if (HAL_FLASH_Unlock() != HAL_OK) {
    return BSP_ERR;
  }
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

  /*  HW needs an aligned addr to program flash, which data
   *  parameters doesn't ensure  */
  /*  case where data is aligned, so let's avoid any copy */
  while ((addr < (startaddr + size))) {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)addr,
                          *((uint64_t*)data)) == HAL_OK) {
      addr = addr + 8;
      data = data + 8;
    } else {
      break;
    }
  }

  HAL_FLASH_Lock();

  return BSP_OK;
}

bsp_status_t bsp_flash_erase(void* addr, size_t size) {
  FLASH_EraseInitTypeDef flash_erase;

  void* startaddr = addr;

  uint32_t sector_error = 0;

  HAL_FLASH_Unlock();

  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

  while (addr < startaddr + size) {
    flash_erase.NbPages = 1;
    flash_erase.Banks = FLASH_BANK_1;
    flash_erase.Page = ((uint32_t)(addr)-FLASH_BASE) / FLASH_PAGE_SIZE;
    flash_erase.TypeErase = FLASH_TYPEERASE_PAGES;
    addr += BSP_FLASH_BLOCK_SIZE;
  }

  /* Erase FLASH*/
  HAL_FLASHEx_Erase(&flash_erase, &sector_error);

  HAL_FLASH_Lock();

  return BSP_OK;
}
