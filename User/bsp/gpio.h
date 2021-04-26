#pragma once

/* Includes ----------------------------------------------------------------- */
#include <stdint.h>

#include "bsp/bsp.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
/* Exported functions prototypes -------------------------------------------- */
int8_t BSP_GPIO_RegisterCallback(uint16_t pin, void (*callback)(void));

int8_t BSP_GPIO_EnableIRQ(uint16_t pin);
int8_t BSP_GPIO_DisableIRQ(uint16_t pin);
