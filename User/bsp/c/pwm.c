/* Includes ----------------------------------------------------------------- */
#include "bsp/pwm.h"

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
    case BSP_PWM_SHOOT_SERVO:
      HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
      break;
  }
  return BSP_OK;
}

int8_t BSP_PWM_Set(BSP_PWM_Channel_t ch, float duty_cycle) {
  if (duty_cycle > 1.0f) return BSP_ERR;
  if (duty_cycle < 0.0f) duty_cycle = 0.f;

  uint16_t pulse;

  /* 通过PWM通道对应定时器重载值和给定占空比，计算PWM周期值 */
  switch (ch) {
    case BSP_PWM_IMU_HEAT:
      pulse = (uint16_t)(duty_cycle * (float)__HAL_TIM_GET_AUTORELOAD(&htim10));
      break;
    case BSP_PWM_SHOOT_SERVO:
      pulse = (uint16_t)(duty_cycle * (float)__HAL_TIM_GET_AUTORELOAD(&htim1));
      break;
  }

  if (pulse > 0) {
    switch (ch) {
      case BSP_PWM_IMU_HEAT:
        __HAL_TIM_SET_COMPARE(&htim10, TIM_CHANNEL_1, pulse);
        break;
      case BSP_PWM_SHOOT_SERVO:
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
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
    case BSP_PWM_SHOOT_SERVO:
      HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
      break;
  }
  HAL_GPIO_WritePin(IMU_HEAT_PWM_GPIO_Port, IMU_HEAT_PWM_Pin, GPIO_PIN_RESET);
  return BSP_OK;
}
