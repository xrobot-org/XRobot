#pragma once

/* Includes ----------------------------------------------------------------- */
#include <stdint.h>

#include "bsp/bsp.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
/* Exported functions prototypes -------------------------------------------- */
uint32_t BSP_GetRandomNum(void);
int32_t BSP_GetRandomRangle(int32_t min, int32_t max);
