#pragma once

#include <stdint.h>

/* LED灯状态，设置用 */
typedef enum {
  LED_ON,
  LED_OFF,
  LED_TAGGLE,
} led_status_t;

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
} led_channel_t;

/* 用于A板时，会无视duty_cycle的值。使用B板时，duty_cycle才有效*/
int8_t led_set(led_channel_t ch, led_status_t s, float duty_cycle);
