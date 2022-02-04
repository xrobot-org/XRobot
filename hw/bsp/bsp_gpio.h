#pragma once

#include <stdint.h>

#include "bsp.h"

int8_t bsp_gpio_register_callback(uint16_t pin, void (*callback)(void *),
                                 void *callback_arg);

int8_t bsp_gpio_enable_irq(uint16_t pin);
int8_t bsp_gpio_disable_irq(uint16_t pin);
