/**
 * @file info.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 指示线程
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 * 控制指示装置，例如LED、OLED显示器等。
 *
 */

#include "bsp_led.h"
#include "comp_capacity.h"
#include "comp_utils.h"
#include "thd.h"

#define THD_PERIOD_MS (250)

void Thd_Info(void *argument) {
  RM_UNUSED(argument); /* 未使用argument，消除警告 */

  /* 计算线程运行到指定频率需要等待的tick数 */
  const uint32_t delay_tick = pdMS_TO_TICKS(THD_PERIOD_MS);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    BSP_LED_Set(BSP_LED_GRN, BSP_LED_TAGGLE, 1); /* 闪烁LED */

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, delay_tick);
  }
}
