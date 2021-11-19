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

#include "comp_capacity.h"
#include "comp_utils.h"
#include "dev_led.h"
#include "thd.h"

#define THD_PERIOD_MS (250)

/* 计算线程运行到指定频率需要等待的tick数 */
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void Thd_Info(void *arg) {
  RM_UNUSED(arg); /* 未使用arg，消除警告 */

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    LED_Set(LED_GRN, LED_TAGGLE, 1); /* 闪烁LED */

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(Thd_Info, 128, 1);
