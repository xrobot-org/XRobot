#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "stm32g4xx_hal.h"

/* 一块FLASH空间的大小 */
#define MF_FLASH_BLOCK_SIZE (2048)

/* 主FLASH地址 */
#define MF_FLASH_MAIN_ADDR (0x0801F000)

/* 备份FLASH地址 */
#define MF_FLASH_BACKUP_ADDR (0x0801F000 + MF_FLASH_BLOCK_SIZE)

/* Flash读写函数 */
#define MF_ERASE mf_erase
#define MF_WRITE mf_write

__attribute__((unused)) static void mf_erase(uint32_t addr) {
  FLASH_EraseInitTypeDef flash_erase;

  uint32_t sector_error = 0;

  HAL_FLASH_Unlock();

  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

  /* You can add your code under here. */
  flash_erase.NbPages = 1;
  flash_erase.Banks = FLASH_BANK_1;
  flash_erase.Page = (addr - FLASH_BASE) / FLASH_PAGE_SIZE;
  flash_erase.TypeErase = FLASH_TYPEERASE_PAGES;

  /* Erase FLASH*/
  HAL_FLASHEx_Erase(&flash_erase, &sector_error);

  HAL_FLASH_Lock();
}

__attribute__((unused)) static void mf_write(uint32_t addr, void *buf) {
  uint32_t startaddr = 0;
  uint8_t *data = buf;

  if (HAL_FLASH_Unlock() != HAL_OK) {
    return;
  }
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

  startaddr = addr;

  /*  HW needs an aligned addr to program flash, which data
   *  parameters doesn't ensure  */
  /*  case where data is aligned, so let's avoid any copy */
  while ((addr < (startaddr + MF_FLASH_BLOCK_SIZE))) {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr,
                          *((uint64_t *)data)) == HAL_OK) {
      addr = addr + 8;
      data = data + 8;
    } else {
      break;
    }
  }

  HAL_FLASH_Lock();
}
