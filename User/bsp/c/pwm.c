/* Includes ----------------------------------------------------------------- */
#include "bsp\pwm.h"

#include <main.h>
#include <tim.h>

/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private typedef ---------------------------------------------------------- */
/* Private variables -------------------------------------------------------- */
/* Private function  -------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */
int8_t BSP_PWM_Start(BSP_PWM_Channel_t ch) {
  switch (ch) {
    case BSP_PWM_IMU_HEAT:
      HAL_TIM_PWM_Start(&htim10, TIM_CHANNEL_1);
      break;
  }
  return BSP_OK;
}

int8_t BSP_PWM_Set(BSP_PWM_Channel_t ch, float duty_cycle) {
  if (duty_cycle > 1.0f) return BSP_ERR;
  if (duty_cycle < 0.0f) duty_cycle = 0.f;

  uint16_t pulse = (uint16_t)(duty_cycle * (float)UINT16_MAX);

  if (pulse > 0) {
    switch (ch) {
      case BSP_PWM_IMU_HEAT:
        __HAL_TIM_SET_COMPARE(&htim10, TIM_CHANNEL_1, pulse);
        break;
    }
  } else {
    BSP_PWM_Stop(ch);
  }
  return BSP_OK;
}

int8_t BSP_PWM_Stop(BSP_PWM_Channel_t ch) {
  switch (ch) {
    case BSP_PWM_IMU_HEAT:
      HAL_TIM_PWM_Stop(&htim10, TIM_CHANNEL_1);
      break;
  }
  HAL_GPIO_WritePin(IMU_HEAT_PWM_GPIO_Port, IMU_HEAT_PWM_Pin, GPIO_PIN_RESET);
  return BSP_OK;
}
