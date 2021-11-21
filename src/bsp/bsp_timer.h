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
} BSP_Timer_t;

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
} BSP_Timer_Callback_t;

TIM_HandleTypeDef *BSP_Timer_GetHandle(BSP_Timer_t timer);
int8_t BSP_Timer_RegisterCallback(BSP_Timer_t timer, BSP_Timer_Callback_t type,
                                  void (*callback)(void));
