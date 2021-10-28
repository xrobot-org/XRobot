#pragma once

#include <stdint.h>

/* LED灯状态，设置用 */
typedef enum {
  LED_ON,
  LED_OFF,
  LED_TAGGLE,
} LED_Status_t;

/* LED通道 */
typedef enum {
#ifdef DEV_BOARD_A
  LED1,
  LED2,
  LED3,
  LED4,
  LED5,
  LED6,
  LED7,
  LED8,
#elif defined BOARD_RM_C
  LED_BLU,
#endif
  LED_RED,
  LED_GRN,
} LED_Channel_t;

/* 用于A板时，会无视duty_cycle的值。使用B板时，duty_cycle才有效*/
int8_t LED_Set(LED_Channel_t ch, LED_Status_t s, float duty_cycle);
