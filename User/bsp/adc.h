#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------- */
#include <stdint.h>

#include "bsp/bsp.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
/* Exported functions prototypes -------------------------------------------- */
float BSP_GetTemperature(void);
float BSP_GetBatteryVolt(void);
uint8_t BSP_GetHardwareVersion(void);

#ifdef __cplusplus
}
#endif
