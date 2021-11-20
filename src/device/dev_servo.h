#pragma once

#include <stdint.h>

/* 舵机通道 */
typedef enum {
  SERVO_A = 0,
  SERVO_B,
  SERVO_C,
  SERVO_D,
  SERVO_E,
  SERVO_F,
  SERVO_G,
#ifdef DEV_BOARD_A
  SERVO_H,
  SERVO_S,
  SERVO_T,
  SERVO_U,
  SERVO_V,
  SERVO_W,
  SERVO_X,
  SERVO_Y,
  SERVO_Z,
#endif
  SERVO_NUM,
} servo_channel_t;

int8_t servo_init(servo_channel_t ch, float max_angle);
int8_t servo_start(servo_channel_t ch);
int8_t servo_set(servo_channel_t ch, uint8_t angle);
int8_t servo_stop(servo_channel_t ch);
