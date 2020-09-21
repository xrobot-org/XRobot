/* Includes ----------------------------------------------------------------- */
#include "bsp\servo.h"

#include <main.h>
#include <tim.h>

/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private typedef ---------------------------------------------------------- */
/* Private variables -------------------------------------------------------- */
static float range[BSP_SERVO_NUM];

/* Private function  -------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */
int8_t BSP_Servo_Init(BSP_Servo_Channel_t ch, float max_angle) {
  range[ch] = max_angle;

  return BSP_OK;
}

int8_t BSP_Servo_Start(BSP_Servo_Channel_t ch) {
  switch (ch) {
    case BSP_SERVO_A:
      HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
      break;

    case BSP_SERVO_B:
      HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
      break;

    case BSP_SERVO_C:
      HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
      break;

    case BSP_SERVO_D:
      HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
      break;

    case BSP_SERVO_E:
      HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
      break;

    case BSP_SERVO_F:
      HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
      break;

    case BSP_SERVO_G:
      HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);

    case BSP_SERVO_H:
      HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
      break;

    case BSP_SERVO_NUM:
      break;
  }
  return BSP_OK;
}

int8_t BSP_Servo_Set(BSP_Servo_Channel_t ch, uint8_t angle) {
  if (angle > 1.0f) return BSP_ERR;

  uint16_t pulse = angle * UINT16_MAX;

  switch (ch) {
    case BSP_SERVO_A:
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse);
      break;

    case BSP_SERVO_B:
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse);
      break;

    case BSP_SERVO_C:
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse);
      break;

    case BSP_SERVO_D:
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse);
      break;

    case BSP_SERVO_E:
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse);
      break;

    case BSP_SERVO_F:
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse);
      break;

    case BSP_SERVO_G:
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse);

    case BSP_SERVO_H:
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse);
      break;

    case BSP_SERVO_NUM:
      break;
  }
  return BSP_OK;
}

int8_t BSP_Servo_Stop(BSP_Servo_Channel_t ch) {
  switch (ch) {
    case BSP_SERVO_A:
      HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_2);
      break;

    case BSP_SERVO_B:
      HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_2);
      break;

    case BSP_SERVO_C:
      HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_2);
      break;

    case BSP_SERVO_D:
      HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_2);
      break;

    case BSP_SERVO_E:
      HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_2);
      break;

    case BSP_SERVO_F:
      HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_2);
      break;

    case BSP_SERVO_G:
      HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_2);

    case BSP_SERVO_H:
      HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_2);
      break;

    case BSP_SERVO_NUM:
      break;
  }
  return BSP_OK;
}
