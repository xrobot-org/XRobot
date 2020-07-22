/* Includes ------------------------------------------------------------------*/
#include "bsp\crc32.h"

#include <crc.h>

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
uint32_t BSP_CRC32_Calc(const uint8_t *buf, size_t len) {
	return HAL_CRC_Calculate(&hcrc, (uint32_t*)buf, len / sizeof(uint32_t));
}

bool BSP_CRC32_Verify(const uint8_t *buf, size_t len) {
	if (len < 2)
		return false;
	
    uint32_t expected = BSP_CRC32_Calc(buf, len / sizeof(uint32_t) - 1);
	
	return expected == ((const uint32_t*)buf)[len / sizeof(uint32_t) - 1];
}

bool BSP_CRC32_Append(const uint8_t *buf, size_t len) {
	return HAL_CRC_Accumulate(&hcrc, (uint32_t*)buf, len / sizeof(uint32_t));
}
