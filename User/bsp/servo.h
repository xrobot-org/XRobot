#pragma once

/* Includes ----------------------------------------------------------------- */
#include <stdint.h>

#include "bsp/bsp.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */

/* 舵机通道 */
typedef enum {
  BSP_SERVO_A = 0,
  BSP_SERVO_B,
  BSP_SERVO_C,
  BSP_SERVO_D,
  BSP_SERVO_E,
  BSP_SERVO_F,
  BSP_SERVO_G,
#ifdef DEV_BOARD_A
  BSP_SERVO_H,
  BSP_SERVO_S,
  BSP_SERVO_T,
  BSP_SERVO_U,
  BSP_SERVO_V,
  BSP_SERVO_W,
  BSP_SERVO_X,
  BSP_SERVO_Y,
  BSP_SERVO_Z,
#endif
  BSP_SERVO_NUM,
} BSP_Servo_Channel_t;

/* Exported functions prototypes -------------------------------------------- */
int8_t BSP_Servo_Init(BSP_Servo_Channel_t ch, float max_angle);
int8_t BSP_Servo_Start(BSP_Servo_Channel_t ch);
int8_t BSP_Servo_Set(BSP_Servo_Channel_t ch, uint8_t angle);
int8_t BSP_Servo_Stop(BSP_Servo_Channel_t ch);
