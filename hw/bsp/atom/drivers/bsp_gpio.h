#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"

typedef enum {
  BSP_GPIO_IMU_CS,
  BSP_GPIO_IMU_INT_1,
  BSP_GPIO_IMU_INT_2,
  BSP_GPIO_LED,
  BSP_GPIO_NUM,
} bsp_gpio_t;

bsp_status_t bsp_gpio_register_callback(bsp_gpio_t gpio,
                                        void (*callback)(void *),
                                        void *callback_arg);

bsp_status_t bsp_gpio_enable_irq(bsp_gpio_t gpio);
bsp_status_t bsp_gpio_disable_irq(bsp_gpio_t gpio);
bsp_status_t bsp_gpio_write_pin(bsp_gpio_t gpio, bool value);
bool bsp_gpio_read_pin(bsp_gpio_t gpio);

#ifdef __cplusplus
}
#endif
