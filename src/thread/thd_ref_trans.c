/**
 * @file referee.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 裁判系统接收发送线程
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 * 接收来自裁判系统的数据
 * 解析后根据需求组合成新包发给各个模块
 * 无论裁判系统是否在线，都要按时发送给各个模块
 *
 */

#include "bsp_usb.h"
#include "dev_referee.h"
#include "mid_msg_dist.h"
#include "thd.h"

#define THD_PERIOD_MS (2)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_ref_trans(void* arg) {
  runtime_t* runtime = arg;
  referee_trans_t ref;

  subscriber_t* ui_cap_sub = msg_dist_subscribe("cap_ui", true);
  subscriber_t* ui_chassis_sub = msg_dist_subscribe("chassis_ui", true);
  subscriber_t* ui_gimbal_sub = msg_dist_subscribe("gimbal_ui", true);
  subscriber_t* ui_launcher_sub = msg_dist_subscribe("launcher_ui", true);

#if UI_MODE_NONE
  vTaskSuspend(xTaskGetCurrentTaskHandle());
#endif

  referee_trans_init(&ref, &(runtime->cfg.pilot_cfg->screen));

  uint32_t previous_wake_time = xTaskGetTickCount();
  while (1) {
    /* 获取其他进程数据用于绘制UI */
    msg_dist_poll(ui_cap_sub, &(ref.cap_ui), 0);
    msg_dist_poll(ui_chassis_sub, &(ref.chassis_ui), 0);
    msg_dist_poll(ui_gimbal_sub, &(ref.gimbal_ui), 0);
    msg_dist_poll(ui_launcher_sub, &(ref.launcher_ui), 0);
    msg_dist_poll(ui_launcher_sub, &(ref.cmd_ui), 0);
#if 0
      xQueueReceive(runtime->msgq.ui.ai, &(ref.ai_ui), 0);
#endif

    /* 刷新UI数据 */
    referee_refresh_ui(&ref);

    if (referee_wait_trans_cplt(&ref, 0)) {
      referee_pack_ui_packet(&ref);
      referee_start_transmit(&ref);
    }

    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}

THREAD_DECLEAR(thd_ref_trans, 512, 4);
