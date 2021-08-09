#pragma once

/* Includes ----------------------------------------------------------------- */
#include <stdint.h>

#include "bsp/bsp.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
/* Exported functions prototypes -------------------------------------------- */
int8_t BSP_Fric_Start(void);
int8_t BSP_Fric_Set(float duty_cycle);
int8_t BSP_Fric_Stop(void);
