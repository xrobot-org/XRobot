/**
 * @file ctrl_cap.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 超级电容控制线程
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "dev_cap.h"
#include "mid_msg_dist.h"
#include "thd.h"

#define THD_PERIOD_MS (10)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_cap(void* arg) {
  RM_UNUSED(arg);

  cap_t cap;
  cap_control_t cap_out;
  ui_cap_t cap_ui;

  publisher_t* ui_pub = msg_dist_create_topic("cap_ui", sizeof(ui_cap_t));
  publisher_t* info_pub = msg_dist_create_topic("cap_info", sizeof(cap_t));

  subscriber_t* out_sub = msg_dist_subscribe("cap_out", sizeof(cap_control_t));

  cap_init(&cap);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取裁判系统信息 */
    if (cap_update(&cap, THD_PERIOD_MS)) {
      /* 一定时间长度内接收不到电容反馈值，使电容离线 */
      cap_handle_offline(&cap);
    }
    cap_pack_ui(&cap, &cap_ui);
    msg_dist_publish(ui_pub, &ui_pub);
    msg_dist_publish(info_pub, &cap);

    msg_dist_poll(out_sub, &cap_out, 0);
    cap_control(&cap, &cap_out);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_cap, 128, 2);
