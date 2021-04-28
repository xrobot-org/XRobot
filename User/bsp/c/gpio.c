/* Includes ----------------------------------------------------------------- */
#include "bsp/gpio.h"

#include <gpio.h>
#include <main.h>

#include "component/utils.h"

/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private typedef ---------------------------------------------------------- */
/* Private variables -------------------------------------------------------- */
static void (*GPIO_Callback[16])(void);

/* Private function  -------------------------------------------------------- */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  for (uint8_t i = 0; i < 16; i++) {
    if (GPIO_Pin & (1 << i)) {
      if (GPIO_Callback[i]) {
        GPIO_Callback[i]();
      }
    }
  }
}

/* Exported functions ------------------------------------------------------- */
int8_t BSP_GPIO_RegisterCallback(uint16_t pin, void (*callback)(void)) {
  ASSERT(callback);

  for (uint8_t i = 0; i < 16; i++) {
    if (pin & (1 << i)) {
      GPIO_Callback[i] = callback;
      break;
    }
  }
  return BSP_OK;
}

int8_t BSP_GPIO_EnableIRQ(uint16_t pin) {
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

int8_t BSP_GPIO_DisableIRQ(uint16_t pin) {
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
