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

void Thd_Cap(void* arg) {
  RM_UNUSED(arg);

  Cap_t cap;
  Cap_Control_t cap_out;
  UI_CapUI_t cap_ui;

  MsgDist_Publisher_t* ui_pub =
      MsgDist_CreateTopic("cap_ui", sizeof(UI_CapUI_t));
  MsgDist_Publisher_t* info_pub =
      MsgDist_CreateTopic("cap_info", sizeof(Cap_t));

  MsgDist_Subscriber_t* out_sub =
      MsgDist_Subscribe("cap_out", sizeof(Cap_Control_t));

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取裁判系统信息 */
    if (Cap_Update(&cap, THD_PERIOD_MS)) {
      /* 一定时间长度内接收不到电容反馈值，使电容离线 */
      Cap_HandleOffline(&cap);
    }
    Cap_PackUI(&cap, &cap_ui);
    MsgDist_Publish(ui_pub, &ui_pub);
    MsgDist_Publish(info_pub, &cap);

    MsgDist_Poll(out_sub, &cap_out, 0);
    Cap_Control(&cap, &cap_out);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(Thd_Cap, 128, 2);
