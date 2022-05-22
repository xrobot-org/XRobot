#pragma once

#include "bsp.h"

typedef enum {
  BSP_GPIO_LED_G,
  BSP_GPIO_LED_B,
  BSP_GPIO_LED_R,
  BSP_GPIO_LASER,
  BSP_GPIO_IMU_HEAT_PWM,
  BSP_GPIO_CMPS_RST,
  BSP_GPIO_CMPS_INT,
  BSP_GPIO_IMU_ACCL_CS,
  BSP_GPIO_IMU_GYRO_CS,
  BSP_GPIO_IMU_ACCL_INT,
  BSP_GPIO_IMU_GYRO_INT,
  BSP_GPIO_NUM,
} bsp_gpio_t;

int8_t bsp_gpio_register_callback(bsp_gpio_t gpio, void (*callback)(void *),
                                  void *callback_arg);

int8_t bsp_gpio_enable_irq(bsp_gpio_t gpio);
int8_t bsp_gpio_disable_irq(bsp_gpio_t gpio);
int8_t bsp_gpio_write_pin(bsp_gpio_t gpio, bool value);
bool bsp_gpio_read_pin(bsp_gpio_t gpio);
