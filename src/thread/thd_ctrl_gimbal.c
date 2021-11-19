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

void Thd_CtrlGimbal(void* arg) {
  Runtime_t* runtime = arg;

  Gimbal_t gimbal;
  CMD_GimbalCmd_t gimbal_cmd;
  Motor_FeedbackGroup_t gimbal_motor;
  Motor_Control_t gimbal_out;
  UI_GimbalUI_t gimbal_ui;

  MsgDist_Publisher_t* out_pub =
      MsgDist_CreateTopic("gimbal_out", sizeof(Motor_Control_t));
  MsgDist_Publisher_t* ui_pub =
      MsgDist_CreateTopic("gimbal_ui", sizeof(UI_GimbalUI_t));

  MsgDist_Subscriber_t* eulr_sub = MsgDist_Subscribe("gimbal_eulr", true);
  MsgDist_Subscriber_t* gyro_sub = MsgDist_Subscribe("gimbal_gyro", true);
  MsgDist_Subscriber_t* motor_sub = MsgDist_Subscribe("gimbal_motor_fb", true);
  MsgDist_Subscriber_t* cmd_sub = MsgDist_Subscribe("cmd_gimbal", true);

  /* 初始化云台 */
  Gimbal_Init(&gimbal, &(runtime->cfg.robot_param->gimbal),
              runtime->cfg.gimbal_limit, 1000.0f / (float)THD_PERIOD_MS);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取控制指令、姿态、IMU、电机反馈 */
    MsgDist_Poll(motor_sub, &gimbal_motor, 0);
    MsgDist_Poll(eulr_sub, &(gimbal.feedback.eulr.imu), 0);
    MsgDist_Poll(gyro_sub, &(gimbal.feedback.gyro), 0);
    MsgDist_Poll(cmd_sub, &gimbal_cmd, 0);

    vTaskSuspendAll(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
    Gimbal_UpdateFeedback(&gimbal, &gimbal_motor);
    Gimbal_Control(&gimbal, &gimbal_cmd, xTaskGetTickCount());
    Gimbal_PackOutput(&gimbal, &gimbal_out);
    Gimbal_PackUi(&gimbal, &gimbal_ui);
    xTaskResumeAll();

    MsgDist_Publish(out_pub, &gimbal_out);
    MsgDist_Publish(ui_pub, &gimbal_ui);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
