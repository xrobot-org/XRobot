/* Includes ------------------------------------------------------------------*/
#include "bsp\crc32.h"

#include "crc.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
uint32_t BSP_GetCrc32CheckSum(uint32_t *data, uint32_t len) {
	return HAL_CRC_Calculate(&hcrc, data, len) == HAL_OK;
}

bool BSP_VerifyCrc32CheckSum(uint32_t *data, uint32_t len) {
	return 1;
}

bool BSP_AppendCrc32CheckSum(uint32_t *data, uint32_t len) {
	return HAL_CRC_Accumulate(&hcrc, data, len) == HAL_OK;
}
