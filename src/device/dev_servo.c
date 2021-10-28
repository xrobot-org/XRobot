#include "dev_servo.h"

#include "hal_tim.h"

static float range[SERVO_NUM];

int8_t Servo_Init(Servo_Channel_t ch, float max_angle) {
  range[ch] = max_angle;

  return 0;
}

int8_t Servo_Start(Servo_Channel_t ch) {
  switch (ch) {
    case SERVO_A:
      HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
      break;

    case SERVO_B:
      HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
      break;

    case SERVO_C:
      HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
      break;

    case SERVO_D:
      HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
      break;

    case SERVO_E:
      HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
      break;

    case SERVO_F:
      HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);
      break;

    case SERVO_G:
      HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3);

    case SERVO_NUM:
      break;
  }
  return 0;
}

int8_t Servo_Set(Servo_Channel_t ch, uint8_t angle) {
  if (angle > 1.0f) return -1;

  uint16_t pulse = angle * UINT16_MAX;

  switch (ch) {
    case SERVO_A:
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
      break;

    case SERVO_B:
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, pulse);
      break;

    case SERVO_C:
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, pulse);
      break;

    case SERVO_D:
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, pulse);
      break;

    case SERVO_E:
      __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_1, pulse);
      break;

    case SERVO_F:
      __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_2, pulse);
      break;

    case SERVO_G:
      __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3, pulse);

    case SERVO_NUM:
      break;
  }
  return 0;
}

int8_t Servo_Stop(Servo_Channel_t ch) {
  switch (ch) {
    case SERVO_A:
      HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
      break;

    case SERVO_B:
      HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
      break;

    case SERVO_C:
      HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
      break;

    case SERVO_D:
      HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
      break;

    case SERVO_E:
      HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_1);
      break;

    case SERVO_F:
      HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_2);
      break;

    case SERVO_G:
      HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_3);

    case SERVO_NUM:
      break;
  }
  return 0;
}
