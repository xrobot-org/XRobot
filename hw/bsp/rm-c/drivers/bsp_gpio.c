#include "bsp_gpio.h"
#include "comp_utils.h"
#include "hal.h"

typedef struct {
  uint16_t pin;
  GPIO_TypeDef *gpio;
} bsp_gpio_map_t;

static const bsp_gpio_map_t bsp_gpio_map[BSP_GPIO_NUM] = {
    {LED_G_Pin, LED_G_GPIO_Port},
    {LED_B_Pin, LED_B_GPIO_Port},
    {LED_R_Pin, LED_R_GPIO_Port},
    {LASER_Pin, LASER_GPIO_Port},
    {IMU_HEAT_PWM_Pin, IMU_HEAT_PWM_GPIO_Port},
    {CMPS_RST_Pin, CMPS_RST_GPIO_Port},
    {CMPS_INT_Pin, CMPS_INT_GPIO_Port},
    {ACCL_CS_Pin, ACCL_CS_GPIO_Port},
    {GYRO_CS_Pin, GYRO_CS_GPIO_Port},
    {ACCL_INT_Pin, ACCL_INT_GPIO_Port},
    {GYRO_INT_Pin, GYRO_INT_GPIO_Port},
};

static bsp_callback_t callback_list[16];

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  for (uint8_t i = 0; i < 16; i++) {
    if (GPIO_Pin & (1 << i)) {
      bsp_callback_t cb = callback_list[i];

      if (cb.fn) {
        cb.fn(cb.arg);
      }
    }
  }
}

int8_t bsp_gpio_register_callback(bsp_gpio_t gpio, void (*callback)(void *),
                                  void *callback_arg) {
  ASSERT(callback);

  uint16_t pin = bsp_gpio_map[gpio].pin;

  for (uint8_t i = 0; i < 16; i++) {
    if (pin & (1 << i)) {
      callback_list[i].fn = callback;
      callback_list[i].arg = callback_arg;
      return BSP_OK;
    }
  }
  return BSP_ERR;
}

int8_t bsp_gpio_enable_irq(bsp_gpio_t gpio) {
  uint16_t pin = bsp_gpio_map[gpio].pin;

  switch (pin) {
    case ACCL_INT_Pin:
      HAL_NVIC_EnableIRQ(ACCL_INT_EXTI_IRQn);
      break;

    case GYRO_INT_Pin:
      HAL_NVIC_EnableIRQ(GYRO_INT_EXTI_IRQn);
      break;

    case USER_KEY_Pin:
      HAL_NVIC_EnableIRQ(USER_KEY_EXTI_IRQn);
      break;

    case CMPS_INT_Pin:
      HAL_NVIC_EnableIRQ(CMPS_INT_EXTI_IRQn);
      break;
      /*
      case XXX_Pin:
              HAL_NVIC_EnableIRQ(XXX_IRQn);
              break;
      */

    default:
      return BSP_ERR;
  }
  return BSP_OK;
}

int8_t bsp_gpio_disable_irq(bsp_gpio_t gpio) {
  uint16_t pin = bsp_gpio_map[gpio].pin;

  switch (pin) {
    case ACCL_INT_Pin:
      HAL_NVIC_DisableIRQ(ACCL_INT_EXTI_IRQn);
      break;

    case GYRO_INT_Pin:
      HAL_NVIC_DisableIRQ(GYRO_INT_EXTI_IRQn);
      break;

    case USER_KEY_Pin:
      HAL_NVIC_DisableIRQ(USER_KEY_EXTI_IRQn);
      break;

    case CMPS_INT_Pin:
      HAL_NVIC_DisableIRQ(CMPS_INT_EXTI_IRQn);
      break;
      /*
      case XXX_Pin:
              HAL_NVIC_EnableIRQ(XXX_IRQn);
              break;
      */

    default:
      return BSP_ERR;
  }
  return BSP_OK;
}

inline int8_t bsp_gpio_write_pin(bsp_gpio_t gpio, bool value) {
  HAL_GPIO_WritePin(bsp_gpio_map[gpio].gpio, bsp_gpio_map[gpio].pin, value);
  return BSP_OK;
}

inline bool bsp_gpio_read_pin(bsp_gpio_t gpio) {
  return HAL_GPIO_ReadPin(bsp_gpio_map[gpio].gpio, bsp_gpio_map[gpio].pin);
}
