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
#include "dev_referee.hpp"
#include "om.h"
#include "thd.hpp"

#define THD_PERIOD_MS (2)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_ref_trans(void* arg) {
  runtime_t* runtime = (runtime_t*)arg;
  referee_trans_t ref;

  om_topic_t* ui_cap_tp = om_find_topic("cap_ui", UINT32_MAX);
  om_topic_t* ui_ch_tp = om_find_topic("chassis_ui", UINT32_MAX);
  om_topic_t* ui_gm_tp = om_find_topic("gimbal_ui", UINT32_MAX);
  om_topic_t* ui_la_tp = om_find_topic("launcher_ui", UINT32_MAX);
  om_topic_t* ui_cmd_tp = om_find_topic("cmd_ui", UINT32_MAX);

  om_suber_t* ui_cap_sub = om_subscript(ui_cap_tp, OM_PRASE_VAR(ref.cap_ui));
  om_suber_t* ui_cmd_sub = om_subscript(ui_cmd_tp, OM_PRASE_VAR(ref.cmd_ui));
  om_suber_t* ui_chassis_sub =
      om_subscript(ui_ch_tp, OM_PRASE_VAR(ref.chassis_ui));
  om_suber_t* ui_gimbal_sub =
      om_subscript(ui_gm_tp, OM_PRASE_VAR(ref.gimbal_ui));
  om_suber_t* ui_launcher_sub =
      om_subscript(ui_la_tp, OM_PRASE_VAR(ref.launcher_ui));

#if UI_MODE_NONE
  vTaskSuspend(xTaskGetCurrentTaskHandle());
#endif

  referee_trans_init(&ref, &(runtime->cfg.pilot_cfg->screen));

  uint32_t previous_wake_time = xTaskGetTickCount();
  while (1) {
    /* 获取其他进程数据用于绘制UI */
    om_suber_export(ui_cap_sub, false);
    om_suber_export(ui_chassis_sub, false);
    om_suber_export(ui_gimbal_sub, false);
    om_suber_export(ui_launcher_sub, false);
    om_suber_export(ui_cmd_sub, false);

    /* 刷新UI数据 */
    referee_refresh_ui(&ref);

    if (!referee_ui_stack_empty(&ref))
      if (referee_wait_trans_cplt(&ref, 0)) {
        referee_pack_ui_packet(&ref);
        referee_start_transmit(&ref);
      }

    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}

THREAD_DECLEAR(thd_ref_trans, 512, 4);
