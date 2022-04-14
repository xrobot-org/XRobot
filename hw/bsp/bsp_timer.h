#pragma once

#include <stdint.h>

#include "bsp.h"
#include "hal_tim.h"

/* 要添加使用SPI的新设备，需要先在此添加对应的枚举值 */

/* SPI实体枚举，与设备对应 */
typedef enum {
  BSP_TIMER_OLED,
  BSP_TIMER_IMU,
  /* BSP_TIMER_XXX,*/
  BSP_TIMER_NUM,
  BSP_TIMER_ERR,
} bsp_timer_t;

/* SPI支持的中断回调函数类型，具体参考HAL中定义 */
typedef enum {
  BSP_TIMER_TX_CPLT_CB,
  BSP_TIMER_RX_CPLT_CB,
  BSP_TIMER_TX_RX_CPLT_CB,
  BSP_TIMER_TX_HALF_CPLT_CB,
  BSP_TIMER_RX_HALF_CPLT_CB,
  BSP_TIMER_TX_RX_HALF_CPLT_CB,
  BSP_TIMER_ERROR_CB,
  BSP_TIMER_ABORT_CPLT_CB,
  BSP_TIMER_CB_NUM,
} bsp_timer_callback_t;

TIM_HandleTypeDef *bsp_timer_get_handle(bsp_timer_t timer);
uint64_t bsp_timer_get_realtime();
int8_t bsp_timer_register_callback(bsp_timer_t timer, bsp_timer_callback_t type,
                                   void (*callback)(void));
