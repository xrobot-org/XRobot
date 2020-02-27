#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
int BSP_GPIO_RegisterCallback(uint16_t pin, void (*callback)(void));

int BSP_GPIO_EnableIRQ(uint16_t pin);
int BSP_GPIO_DisableIRQ(uint16_t pin);
