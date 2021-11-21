#pragma once

#include <stdint.h>

#include "bsp.h"

int8_t BSP_GPIO_RegisterCallback(uint16_t pin, void (*callback)(void *),
                                 void *callback_arg);

int8_t BSP_GPIO_EnableIRQ(uint16_t pin);
int8_t BSP_GPIO_DisableIRQ(uint16_t pin);
