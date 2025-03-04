#include "bsp_gpio.h"

#include "main.h"

typedef struct {
  uint16_t pin;
  GPIO_TypeDef *gpio;
} bsp_gpio_map_t;

static const bsp_gpio_map_t BSP_GPIO_MAP[BSP_GPIO_NUM] = {
    {IMU_CS_Pin, IMU_CS_GPIO_Port},
    {IMU_INT1_Pin, IMU_INT1_GPIO_Port},
    {BMI088_CS_1_Pin, BMI088_CS_1_GPIO_Port},
    {BMI088_CS_2_Pin, BMI088_CS_2_GPIO_Port},
    {BMI088_INT_1_Pin, BMI088_INT_1_GPIO_Port},
    {BMI088_INT_2_Pin, BMI088_INT_2_GPIO_Port},
    {LED_RUN_Pin, LED_RUN_GPIO_Port},
};

static bsp_callback_t callback_list[16];

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  for (uint8_t i = 0; i < 16; i++) {
    if (GPIO_Pin & (1 << i)) {
      __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
      bsp_callback_t cb = callback_list[i];

      if (cb.fn) {
        cb.fn(cb.arg);
      }
    }
  }
}

bsp_status_t bsp_gpio_register_callback(bsp_gpio_t gpio,
                                        void (*callback)(void *),
                                        void *callback_arg) {
  assert_param(callback);

  uint16_t pin = BSP_GPIO_MAP[gpio].pin;

  for (uint8_t i = 0; i < 16; i++) {
    if (pin & (1 << i)) {
      callback_list[i].fn = callback;
      callback_list[i].arg = callback_arg;
      return BSP_OK;
    }
  }
  return BSP_ERR;
}

bsp_status_t bsp_gpio_enable_irq(bsp_gpio_t gpio) {
  switch (gpio) {
    case BSP_GPIO_IMU_INT_1:
      HAL_NVIC_EnableIRQ(IMU_INT1_EXTI_IRQn);
      break;
    case BSP_GPIO_IMU_ACCL_INT:
      HAL_NVIC_EnableIRQ(BMI088_INT_1_EXTI_IRQn);
      break;
    case BSP_GPIO_IMU_GYRO_INT:
      HAL_NVIC_EnableIRQ(BMI088_INT_2_EXTI_IRQn);
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

bsp_status_t bsp_gpio_disable_irq(bsp_gpio_t gpio) {
  switch (gpio) {
    case BSP_GPIO_IMU_INT_1:
      HAL_NVIC_DisableIRQ(IMU_INT1_EXTI_IRQn);
      break;

    case BSP_GPIO_IMU_ACCL_INT:
      HAL_NVIC_DisableIRQ(BMI088_INT_1_EXTI_IRQn);
      break;
    case BSP_GPIO_IMU_GYRO_INT:
      HAL_NVIC_DisableIRQ(BMI088_INT_2_EXTI_IRQn);
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

inline bsp_status_t bsp_gpio_write_pin(bsp_gpio_t gpio, bool value) {
  HAL_GPIO_WritePin(BSP_GPIO_MAP[gpio].gpio, BSP_GPIO_MAP[gpio].pin, value);
  return BSP_OK;
}

inline bool bsp_gpio_read_pin(bsp_gpio_t gpio) {
  return HAL_GPIO_ReadPin(BSP_GPIO_MAP[gpio].gpio, BSP_GPIO_MAP[gpio].pin);
}
