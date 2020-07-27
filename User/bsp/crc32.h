#pragma once


/* Includes ------------------------------------------------------------------*/
#include "component\user_math.h"

#include <stdbool.h>
#include <stdint.h>

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
uint32_t BSP_CRC32_Calc(uint8_t *buf, size_t len);
bool BSP_CRC32_Verify(uint8_t *buf, size_t len);
bool BSP_CRC32_Append(uint8_t *buf, size_t len);
