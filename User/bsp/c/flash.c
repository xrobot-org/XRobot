/* Includes ------------------------------------------------------------------*/
#include "bsp\flash.h"

#include "main.h"

#include <string.h>

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
static uint32_t BSP_Flash_GerSector(uint32_t address) {
	uint32_t sector = NULL;
	if ((address < ADDR_FLASH_SECTOR_1) && (address >= ADDR_FLASH_SECTOR_0)) {
		sector = ADDR_FLASH_SECTOR_0;
	} else if ((address < ADDR_FLASH_SECTOR_2) && (address >= ADDR_FLASH_SECTOR_1)) {
		sector = ADDR_FLASH_SECTOR_1;
	} else if ((address < ADDR_FLASH_SECTOR_3) && (address >= ADDR_FLASH_SECTOR_2)) {
		sector = ADDR_FLASH_SECTOR_2;
	} else if ((address < ADDR_FLASH_SECTOR_4) && (address >= ADDR_FLASH_SECTOR_3)) {
		sector = ADDR_FLASH_SECTOR_3;
	} else if ((address < ADDR_FLASH_SECTOR_5) && (address >= ADDR_FLASH_SECTOR_4)) {
		sector = ADDR_FLASH_SECTOR_4;
	} else if ((address < ADDR_FLASH_SECTOR_6) && (address >= ADDR_FLASH_SECTOR_5)) {
		sector = ADDR_FLASH_SECTOR_5;
	} else if ((address < ADDR_FLASH_SECTOR_7) && (address >= ADDR_FLASH_SECTOR_6)) {
		sector = ADDR_FLASH_SECTOR_6;
	} else if ((address < ADDR_FLASH_SECTOR_8) && (address >= ADDR_FLASH_SECTOR_7)) {
		sector = ADDR_FLASH_SECTOR_7;
	} else if ((address < ADDR_FLASH_SECTOR_9) && (address >= ADDR_FLASH_SECTOR_8)) {
		sector = ADDR_FLASH_SECTOR_8;
	} else if ((address < ADDR_FLASH_SECTOR_10) && (address >= ADDR_FLASH_SECTOR_9)) {
		sector = ADDR_FLASH_SECTOR_9;
	} else if ((address < ADDR_FLASH_SECTOR_11) && (address >= ADDR_FLASH_SECTOR_10)) {
		sector = ADDR_FLASH_SECTOR_10;
#ifdef FALSH_SIZE_LARGE
	} else if ((address < ADDR_FLASH_SECTOR_12) && (address >= ADDR_FLASH_SECTOR_11)) {
		sector = ADDR_FLASH_SECTOR_11;
	} else if ((address < ADDR_FLASH_SECTOR_13) && (address >= ADDR_FLASH_SECTOR_12)) {
		sector = ADDR_FLASH_SECTOR_12;
	} else if ((address < ADDR_FLASH_SECTOR_14) && (address >= ADDR_FLASH_SECTOR_13)) {
		sector = ADDR_FLASH_SECTOR_13;
	} else if ((address < ADDR_FLASH_SECTOR_15) && (address >= ADDR_FLASH_SECTOR_14)) {
		sector = ADDR_FLASH_SECTOR_14;
	} else if ((address < ADDR_FLASH_SECTOR_16) && (address >= ADDR_FLASH_SECTOR_15)) {
		sector = ADDR_FLASH_SECTOR_15;
	} else if ((address < ADDR_FLASH_SECTOR_17) && (address >= ADDR_FLASH_SECTOR_16)) {
		sector = ADDR_FLASH_SECTOR_16;
	} else if ((address < ADDR_FLASH_SECTOR_18) && (address >= ADDR_FLASH_SECTOR_17)) {
		sector = ADDR_FLASH_SECTOR_17;
	} else if ((address < ADDR_FLASH_SECTOR_19) && (address >= ADDR_FLASH_SECTOR_18)) {
		sector = ADDR_FLASH_SECTOR_18;
	} else if ((address < ADDR_FLASH_SECTOR_20) && (address >= ADDR_FLASH_SECTOR_19)) {
		sector = ADDR_FLASH_SECTOR_19;
	} else if ((address < ADDR_FLASH_SECTOR_21) && (address >= ADDR_FLASH_SECTOR_20)) {
		sector = ADDR_FLASH_SECTOR_20;
	} else if ((address < ADDR_FLASH_SECTOR_22) && (address >= ADDR_FLASH_SECTOR_21)) {
		sector = ADDR_FLASH_SECTOR_21;
	} else if ((address < ADDR_FLASH_SECTOR_23) && (address >= ADDR_FLASH_SECTOR_22)) {
		sector = ADDR_FLASH_SECTOR_22;
	} else if ((address < ADDR_FLASH_END) && (address >= ADDR_FLASH_SECTOR_23)) {
		sector = ADDR_FLASH_SECTOR_23;
#else 
	} else if ((address < ADDR_FLASH_END) && (address >= ADDR_FLASH_SECTOR_11)) {
		sector = ADDR_FLASH_SECTOR_11;
	}
#endif
	return sector;
}

void BSP_Flash_EraseSectorAt(uint32_t address) {
	FLASH_EraseInitTypeDef flash_erase;
	uint32_t sector_error;
	
	flash_erase.Sector = BSP_Flash_GerSector(address);
	flash_erase.TypeErase = FLASH_TYPEERASE_SECTORS;
	flash_erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
	flash_erase.NbSectors = 1;
	
	HAL_FLASH_Unlock();
	HAL_FLASHEx_Erase(&flash_erase, &sector_error);
	HAL_FLASH_Lock();
}

void BSP_Flash_WriteBytes(uint32_t address, const uint8_t *buf, size_t len) {
	HAL_FLASH_Unlock();
	for (uint8_t i = 0; i < len; i++) {
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, address, *buf);
		buf++;
	}
	HAL_FLASH_Lock();
}

void BSP_Flash_ReadBytes(uint32_t address, void *buf, size_t len) {
	memcpy(buf, (void*)address, len);
}
