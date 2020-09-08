/* Includes ------------------------------------------------------------------*/
#include "bsp\flash.h"

#include <main.h>
#include <string.h>

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

void BSP_Flash_EraseSector(uint32_t sector) {
  FLASH_EraseInitTypeDef flash_erase;
  uint32_t sector_error;

#ifdef DEV_BOARD_C
  if (sector > 0 && sector < 12) {
#elif
  if (sector > 0 && sector < 24) {
#endif
    flash_erase.Sector = sector;
    flash_erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    flash_erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    flash_erase.NbSectors = 1;

    HAL_FLASH_Unlock();
    while (FLASH_WaitForLastOperation(50) != HAL_OK)
      ;
    HAL_FLASHEx_Erase(&flash_erase, &sector_error);
    HAL_FLASH_Lock();
  }
}

void BSP_Flash_WriteBytes(uint32_t address, const uint8_t *buf, size_t len) {
  HAL_FLASH_Unlock();
  while (len > 0) {
    while (FLASH_WaitForLastOperation(50) != HAL_OK)
      ;
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, address, *buf);
    address++;
    buf++;
    len--;
  }
  HAL_FLASH_Lock();
}

void BSP_Flash_ReadBytes(uint32_t address, void *buf, size_t len) {
  memcpy(buf, (void *)address, len);
}
