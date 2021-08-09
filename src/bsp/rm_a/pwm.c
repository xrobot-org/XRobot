/* Includes ----------------------------------------------------------------- */
#include "bsp_pwm.h"

#include "main.h"
#include "tim.h"

/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private typedef ---------------------------------------------------------- */
/* Private variables -------------------------------------------------------- */
/* Private function  -------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */
uint8_t BSP_PWM_Start(BSP_PWM_Channel_t ch) {
  switch (ch) {
    case BSP_PWM_IMU_HEAT:
      HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
      break;
  }
  return 0;
}

int8_t BSP_PWM_Set(BSP_PWM_Channel_t ch, float duty_cycle) {
  if (duty_cycle > 1.0f) return -1;

  uint16_t pulse = duty_cycle * PWM_RESOLUTION;

  switch (ch) {
    case BSP_PWM_IMU_HEAT:
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse);
      break;
    default:
      return -1;
  }
  return 0;
}

uint8_t BSP_PWM_Stop(BSP_PWM_Channel_t ch);
