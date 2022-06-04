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

#include "dev_can.h"
#include "dev_cap.h"
#include "om.h"
#include "thd.h"

#define THD_PERIOD_MS (250)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_cap(void* arg) {
  runtime_t* runtime = arg;

  cap_t cap;
  cap_control_t cap_out;
  ui_cap_t cap_ui;

  om_topic_t* ui_tp = om_config_topic(NULL, "A", "cap_ui");
  om_topic_t* info_tp = om_config_topic(NULL, "A", "cap_info");
  om_topic_t* out_tp = om_find_topic("cap_out", UINT32_MAX);

  om_suber_t* out_sub = om_subscript(out_tp, OM_PRASE_VAR(cap_out));

  cap_init(&cap, &(runtime->cfg.robot_param->cap));

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取裁判系统信息 */
    if (cap_update(&cap, THD_PERIOD_MS)) {
      /* 一定时间长度内接收不到电容反馈值，使电容离线 */
      cap_handle_offline(&cap);
    }
    cap_pack_ui(&cap, &cap_ui);
    om_publish(ui_tp, OM_PRASE_VAR(cap_ui), true, false);
    om_publish(info_tp, OM_PRASE_VAR(cap.feedback), true, false);

    om_suber_export(out_sub, false);
    cap_control(&cap, &cap_out);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_cap, 256, 2);
