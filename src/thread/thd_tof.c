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

void thd_tof(void* arg) {
  runtime_t* runtime = arg;

  tof_t tof;

  publisher_t* tof_fb_pub = msg_dist_create_topic("tof_fb", sizeof(tof_t));

  tof_init(&tof, &(runtime->cfg.robot_param->tof));

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    if (tof_update(&tof, THD_PERIOD_MS)) {
      tof_handle_offline(&tof);
    }

    msg_dist_publish(tof_fb_pub, &tof);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_tof, 512, 4);
