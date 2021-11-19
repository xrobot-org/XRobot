/**
 * @file thd_tof.c
 * @author Qu Shen
 * @brief
 * @version 1.0.0
 * @date 2021-11-14
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "dev_tof.h"
#include "mid_msg_dist.h"
#include "thd.h"

#define THD_PERIOD_MS (2)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void Thd_TOF(void* arg) {
  RM_UNUSED(arg);

  TOF_t tof;

  MsgDist_Publisher_t* tof_fb_pub =
      MsgDist_CreateTopic("tof_fb", sizeof(TOF_t));

  TOF_Init(&tof);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    if (TOF_Update(&tof, THD_PERIOD_MS)) {
      TOF_HandleOffline(&tof);
    }

    MsgDist_Publish(tof_fb_pub, &tof);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(Thd_TOF, 512, 4);
