/**
 * @file ctrl_gimbal.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 云台控制线程
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "mod_gimbal.h"
#include "om.h"
#include "thd.h"

#define THD_PERIOD_MS (2)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_ctrl_gimbal(void* arg) {
  runtime_t* runtime = arg;

  gimbal_t gimbal;
  cmd_gimbal_t gimbal_cmd;
  motor_feedback_group_t gimbal_yaw_motor;
  motor_feedback_group_t gimbal_pit_motor;
  motor_control_t gimbal_yaw_out;
  motor_control_t gimbal_pit_out;
  ui_gimbal_t gimbal_ui;

  om_topic_t* out_yaw_tp = om_config_topic(NULL, "A", "gimbal_yaw_out");
  om_topic_t* out_pit_tp = om_config_topic(NULL, "A", "gimbal_pit_out");
  om_topic_t* ui_tp = om_config_topic(NULL, "A", "gimbal_ui");
  om_topic_t* eulr_tp = om_find_topic("gimbal_eulr", UINT32_MAX);
  om_topic_t* gyro_tp = om_find_topic("gimbal_gyro", UINT32_MAX);
  om_topic_t* mt_yaw_tp = om_find_topic("gimbal_yaw_motor_fb", UINT32_MAX);
  om_topic_t* mt_pit_tp = om_find_topic("gimbal_pit_motor_fb", UINT32_MAX);
  om_topic_t* cmd_tp = om_find_topic("cmd_gimbal", UINT32_MAX);

  om_suber_t* eulr_sub =
      om_subscript(eulr_tp, OM_PRASE_VAR(gimbal.feedback.eulr.imu), NULL);
  om_suber_t* gyro_sub =
      om_subscript(gyro_tp, OM_PRASE_VAR(gimbal.feedback.gyro), NULL);
  om_suber_t* motor_yaw_sub =
      om_subscript(mt_yaw_tp, OM_PRASE_VAR(gimbal_yaw_motor), NULL);
  om_suber_t* motor_pit_sub =
      om_subscript(mt_pit_tp, OM_PRASE_VAR(gimbal_pit_motor), NULL);
  om_suber_t* cmd_sub = om_subscript(cmd_tp, OM_PRASE_VAR(gimbal_cmd), NULL);

  /* 初始化云台 */
  gimbal_init(&gimbal, &(runtime->cfg.robot_param->gimbal),
              runtime->cfg.gimbal_limit, &(runtime->cfg.gimbal_mech_zero),
              1000.0f / (float)THD_PERIOD_MS);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取控制指令、姿态、IMU、电机反馈 */
    om_suber_dump(motor_yaw_sub, false);
    om_suber_dump(motor_pit_sub, false);
    om_suber_dump(eulr_sub, false);
    om_suber_dump(gyro_sub, false);
    om_suber_dump(cmd_sub, false);

    vTaskSuspendAll(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
    gimbal_update_feedback(&gimbal, &gimbal_yaw_motor, &gimbal_pit_motor);
    gimbal_control(&gimbal, &gimbal_cmd, xTaskGetTickCount());
    gimbal_pack_output(&gimbal, &gimbal_pit_out, &gimbal_yaw_out);
    gimbal_pack_ui(&gimbal, &gimbal_ui);
    xTaskResumeAll();

    om_publish(out_yaw_tp, OM_PRASE_VAR(gimbal_yaw_out), true, false);
    om_publish(out_pit_tp, OM_PRASE_VAR(gimbal_pit_out), true, false);
    om_publish(ui_tp, OM_PRASE_VAR(gimbal_ui), true, false);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_ctrl_gimbal, 384, 2);
