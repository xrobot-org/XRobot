/**
 * @file thd_msg_distrib.c
 * @author Qu Shen
 * @brief 消息分发线程
 * @version 0.1
 * @date 2021-09-05
 *
 * @copyright Copyright (c) 2021
 *
 */

#include <string.h>

#include "bsp_pwm.h"
#include "bsp_usb.h"
#include "comp_ahrs.h"
#include "comp_pid.h"
#include "dev_bmi088.h"
#include "dev_ist8310.h"
#include "om.h"
#include "thd.h"

#define THD_PERIOD_MS (1)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_msg_dist(void *arg) {
  RM_UNUSED(arg); /* 未使用arg，消除警告 */

  /* 初始化消息框架 */
  om_init();

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 消息同步 */
    om_sync(false);

    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_msg_dist, 256, 5);
