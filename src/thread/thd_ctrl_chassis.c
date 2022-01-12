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

#include "comp_limiter.h"
#include "dev_tof.h"
#include "mid_msg_dist.h"
#include "mod_chassis.h"
#include "mod_config.h"
#include "thd.h"

#define THD_PERIOD_MS (2)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_ctrl_chassis(void* arg) {
  runtime_t* runtime = arg;

  chassis_t chassis;

  cap_feedback_t cap;
  motor_feedback_group_t chassis_motor;
  motor_feedback_group_t gimbal_motor;

  cmd_chassis_t chassis_cmd;
  referee_for_chassis_t ref_chassis;
  tof_t tof;

  motor_control_t motor_ctrl;
  cap_control_t cap_ctrl;
  ui_chassis_t chassis_ui;

  publisher_t* motor_pub =
      msg_dist_create_topic("chassis_out", sizeof(motor_control_t));
  publisher_t* cap_pub =
      msg_dist_create_topic("cap_out", sizeof(cap_control_t));
  publisher_t* ui_pub =
      msg_dist_create_topic("chassis_ui", sizeof(ui_chassis_t));

  subscriber_t* ref_sub = msg_dist_subscribe("referee_chassis", true);
  subscriber_t* cap_sub = msg_dist_subscribe("chassis_gyro", true);
  subscriber_t* chassis_motor_sub =
      msg_dist_subscribe("chassis_motor_fb", true);
  subscriber_t* gimbal_motor_sub = msg_dist_subscribe("gimbal_motor_fb", true);
  subscriber_t* cmd_sub = msg_dist_subscribe("cmd_chassis", true);
  subscriber_t* tof_sub = msg_dist_subscribe("tof_fb", true);

  /* 初始化底盘 */
  chassis_init(&chassis, &(runtime->cfg.robot_param->chassis),
               &(runtime->cfg.gimbal_mech_zero),
               1000.0f / (float)THD_PERIOD_MS);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取控制指令、电容、裁判系统、电机反馈 */
    msg_dist_poll(chassis_motor_sub, &chassis_motor, 0);
    msg_dist_poll(gimbal_motor_sub, &gimbal_motor, 0);
    msg_dist_poll(ref_sub, &ref_chassis, 0);
    msg_dist_poll(cmd_sub, &chassis_cmd, 0);
    msg_dist_poll(cap_sub, &cap, 0);
    msg_dist_poll(tof_sub, &tof, 0);
    vTaskSuspendAll(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
    /* 更新反馈值 */
    chassis_update_feedback(&chassis, &chassis_motor, &gimbal_motor);
    chassis_control(&chassis, &chassis_cmd, xTaskGetTickCount());
    chassis_power_limit(&chassis, &cap, &ref_chassis); /* 限制输出功率 */
    chassis_pack_output(&chassis, &motor_ctrl, &cap_ctrl);
    chassis_pack_ui(&chassis, &chassis_ui);
    xTaskResumeAll();

    msg_dist_publish(motor_pub, &motor_ctrl);
    msg_dist_publish(cap_pub, &cap_ctrl);
    msg_dist_publish(ui_pub, &chassis_ui);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_ctrl_chassis, 256, 2);
