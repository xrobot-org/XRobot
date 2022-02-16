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

#include "mid_msg_dist.h"
#include "mod_gimbal.h"
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

  publisher_t* out_yaw_pub =
      msg_dist_create_topic("gimbal_yaw_out", sizeof(motor_control_t));
  publisher_t* out_pit_pub =
      msg_dist_create_topic("gimbal_pit_out", sizeof(motor_control_t));
  publisher_t* ui_pub = msg_dist_create_topic("gimbal_ui", sizeof(ui_gimbal_t));

  subscriber_t* eulr_sub = msg_dist_subscribe("gimbal_eulr", true);
  subscriber_t* gyro_sub = msg_dist_subscribe("gimbal_gyro", true);
  subscriber_t* motor_yaw_sub = msg_dist_subscribe("gimbal_yaw_motor_fb", true);
  subscriber_t* motor_pit_sub = msg_dist_subscribe("gimbal_pit_motor_fb", true);
  subscriber_t* cmd_sub = msg_dist_subscribe("cmd_gimbal", true);

  /* 初始化云台 */
  gimbal_init(&gimbal, &(runtime->cfg.robot_param->gimbal),
              runtime->cfg.gimbal_limit, &(runtime->cfg.gimbal_mech_zero),
              1000.0f / (float)THD_PERIOD_MS);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取控制指令、姿态、IMU、电机反馈 */
    msg_dist_poll(motor_yaw_sub, &gimbal_yaw_motor, 0);
    msg_dist_poll(motor_pit_sub, &gimbal_pit_motor, 0);
    msg_dist_poll(eulr_sub, &(gimbal.feedback.eulr.imu), 0);
    msg_dist_poll(gyro_sub, &(gimbal.feedback.gyro), 0);
    msg_dist_poll(cmd_sub, &gimbal_cmd, 0);

    vTaskSuspendAll(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
    gimbal_update_feedback(&gimbal, &gimbal_yaw_motor, &gimbal_pit_motor);
    gimbal_control(&gimbal, &gimbal_cmd, xTaskGetTickCount());
    gimbal_pack_output(&gimbal, &gimbal_pit_out, &gimbal_yaw_out);
    gimbal_pack_ui(&gimbal, &gimbal_ui);
    xTaskResumeAll();

    msg_dist_publish(out_yaw_pub, &gimbal_yaw_out);
    msg_dist_publish(out_pit_pub, &gimbal_pit_out);
    msg_dist_publish(ui_pub, &gimbal_ui);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_ctrl_gimbal, 384, 2);
