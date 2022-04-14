/**
 * @file monitor.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 监控线程
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 * 监控系统运行情况，记录错误。
 *
 */

#include "bsp_wdg.h"
#include "thd.h"

#define THD_PERIOD_MS (10)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_monitor(void* arg) {
  RM_UNUSED(arg);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
#if !MCU_DEBUG_BUILD
    bsp_wdg_refresh();
#endif

#if USB_REPORT
    om_send_report_data();
#endif

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_monitor, 512, 1);
