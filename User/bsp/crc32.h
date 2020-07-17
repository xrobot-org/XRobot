#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
uint32_t BSP_GetCrc32CheckSum(uint32_t *data, uint32_t len);
bool BSP_VerifyCrc32CheckSum(uint32_t *data, uint32_t len);
bool BSP_AppendCrc32CheckSum(uint32_t *data, uint32_t len);
