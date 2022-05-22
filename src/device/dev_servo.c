#include "dev_servo.h"

#include "bsp_pwm.h"

static float range[SERVO_NUM];

int8_t servo_init(servo_channel_t ch, float max_angle) {
  range[ch] = max_angle;

  return 0;
}

int8_t servo_start(servo_channel_t ch) {
  bsp_pwm_start((bsp_pwm_channel_t)(ch + BSP_PWM_SERVO_A));
  return 0;
}

int8_t servo_set(servo_channel_t ch, uint8_t angle) {
  if (angle > 1.0f) return -1;
  // TODO: 正确的角度转换
  bsp_pwm_set_comp((bsp_pwm_channel_t)(ch + BSP_PWM_SERVO_A), angle);
  return 0;
}

int8_t servo_stop(servo_channel_t ch) {
  bsp_pwm_stop((bsp_pwm_channel_t)(ch + BSP_PWM_SERVO_A));

  return 0;
}
