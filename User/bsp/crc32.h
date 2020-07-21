#pragma once


/* Includes ------------------------------------------------------------------*/
#include "component\user_math.h"

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
uint32_t BSP_CRC32_Calc(const uint8_t *buf, size_t len);
bool BSP_CRC32_Verify(const uint8_t *buf, size_t len);
bool BSP_CRC32_Append(const uint8_t *buf, size_t len);
