/**
 * @file ctrl_chassis.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 底盘控制线程
 * @version 1.0.0
 * @date 2021-04-14
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "comp_limiter.hpp"
#include "mod_chassis.hpp"
#include "mod_config.hpp"
#include "om.h"
#include "thd.hpp"

#define THD_PERIOD_MS (2)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_ctrl_chassis(void* arg) {
  runtime_t* runtime = (runtime_t*)arg;

  chassis_t chassis;

  cap_feedback_t cap;
  tof_feedback_t tof;
  motor_feedback_group_t ch_mt;
  motor_feedback_group_t gm_mt;

  cmd_chassis_t chassis_cmd;
  referee_for_chassis_t ref_chassis;

  motor_control_t motor_ctrl;
  cap_control_t cap_ctrl;
  ui_chassis_t chassis_ui;

  om_topic_t* out_tp = om_config_topic(NULL, "A", "chassis_out");
  om_topic_t* cap_tp = om_config_topic(NULL, "A", "cap_out");
  om_topic_t* ui_tp = om_config_topic(NULL, "A", "chassis_ui");
  om_topic_t* ref_tp = om_find_topic("referee_chassis", UINT32_MAX);
  om_topic_t* cap_info_tp = om_find_topic("cap_info", UINT32_MAX);
  om_topic_t* ch_mt_tp = om_find_topic("chassis_motor_fb", UINT32_MAX);
  om_topic_t* gm_mt_tp = om_find_topic("gimbal_yaw_motor_fb", UINT32_MAX);
  om_topic_t* cmd_tp = om_find_topic("cmd_chassis", UINT32_MAX);
  om_topic_t* tof_tp = om_find_topic("tof_fb", UINT32_MAX);

  om_suber_t* ref_sub = om_subscript(ref_tp, OM_PRASE_VAR(ref_chassis));
  om_suber_t* cap_sub = om_subscript(cap_info_tp, OM_PRASE_VAR(cap));
  om_suber_t* ch_mt_sub = om_subscript(ch_mt_tp, OM_PRASE_VAR(ch_mt));
  om_suber_t* gm_mt_sub = om_subscript(gm_mt_tp, OM_PRASE_VAR(gm_mt));
  om_suber_t* cmd_sub = om_subscript(cmd_tp, OM_PRASE_VAR(chassis_cmd));
  om_suber_t* tof_sub = om_subscript(tof_tp, OM_PRASE_VAR(tof));

  /* 初始化底盘 */
  chassis_init(&chassis, &(runtime->cfg.robot_param->chassis),
               &(runtime->cfg.gimbal_mech_zero),
               1000.0f / (float)THD_PERIOD_MS);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取控制指令、电容、裁判系统、电机反馈 */
    om_suber_export(ch_mt_sub, false);
    om_suber_export(gm_mt_sub, false);
    om_suber_export(ref_sub, false);
    om_suber_export(cmd_sub, false);
    om_suber_export(cap_sub, false);
    om_suber_export(tof_sub, false);
    vTaskSuspendAll(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
    /* 更新反馈值 */
    chassis_update_feedback(&chassis, &ch_mt, &gm_mt, &tof);
    chassis_control(&chassis, &chassis_cmd, xTaskGetTickCount());
    chassis_power_limit(&chassis, &cap, &ref_chassis); /* 限制输出功率 */
    chassis_pack_output(&chassis, &motor_ctrl, &cap_ctrl);
    chassis_pack_ui(&chassis, &chassis_ui);
    xTaskResumeAll();

    om_publish(out_tp, OM_PRASE_VAR(motor_ctrl), true, false);
    om_publish(cap_tp, OM_PRASE_VAR(cap_ctrl), true, false);
    om_publish(ui_tp, OM_PRASE_VAR(chassis_ui), true, false);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_ctrl_chassis, 384, 2);
