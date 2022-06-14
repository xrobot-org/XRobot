#include "dev_can.h"
#include "thd.h"

#define THD_PERIOD_MS (1000)

/* 计算线程运行到指定频率需要等待的tick数 */
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_can(void *arg) {
  RM_UNUSED(arg); /* 未使用arg，消除警告 */

  can_init();

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_can, 128, 4);
