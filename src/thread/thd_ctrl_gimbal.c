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

#include "mid_msg_distrib.h"
#include "mod_gimbal.h"
#include "thd.h"

#ifdef MCU_DEBUG_BUILD

Gimbal_t gimbal;
CMD_GimbalCmd_t gimbal_cmd;
CAN_GimbalMotor_t gimbal_motor;
CAN_GimbalOutput_t gimbal_out;
UI_GimbalUI_t gimbal_ui;

#else

static Gimbal_t gimbal;
static CMD_GimbalCmd_t gimbal_cmd;
static CAN_GimbalMotor_t gimbal_motor;
static CAN_GimbalOutput_t gimbal_out;
static UI_GimbalUI_t gimbal_ui;

#endif

#define THD_PERIOD_MS (2)

void Thd_CtrlGimbal(void* argument) {
  Runtime_t* runtime = argument;
  const uint32_t delay_tick = pdMS_TO_TICKS(THD_PERIOD_MS);

  MsgDistrib_Publisher_t* out_pub =
      MsgDistrib_CreateTopic("gimbal_out", sizeof(CAN_GimbalOutput_t));
  MsgDistrib_Publisher_t* ui_pub =
      MsgDistrib_CreateTopic("gimbal_ui", sizeof(UI_GimbalUI_t));

  MsgDistrib_Subscriber_t* eulr_sub = MsgDistrib_Subscribe("gimbal_eulr", true);
  MsgDistrib_Subscriber_t* gyro_sub = MsgDistrib_Subscribe("gimbal_gyro", true);
  MsgDistrib_Subscriber_t* motor_sub =
      MsgDistrib_Subscribe("gimbal_motor_fb", true);
  MsgDistrib_Subscriber_t* cmd_sub = MsgDistrib_Subscribe("cmd_gimbal", true);

  /* 初始化云台 */
  Gimbal_Init(&gimbal, &(runtime->cfg.robot_param->gimbal),
              runtime->cfg.gimbal_limit, 1000.0f / (float)THD_PERIOD_MS);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取控制指令、姿态、IMU、电机反馈 */
    MsgDistrib_Poll(motor_sub, &gimbal_motor, 0);
    MsgDistrib_Poll(eulr_sub, &(gimbal.feedback.eulr.imu), 0);
    MsgDistrib_Poll(gyro_sub, &(gimbal.feedback.gyro), 0);
    MsgDistrib_Poll(cmd_sub, &gimbal_cmd, 0);

    vTaskSuspendAll(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
    Gimbal_UpdateFeedback(&gimbal, &gimbal_motor);
    Gimbal_Control(&gimbal, &gimbal_cmd, xTaskGetTickCount());
    Gimbal_PackOutput(&gimbal, &gimbal_out);
    Gimbal_PackUi(&gimbal, &gimbal_ui);
    xTaskResumeAll();

    MsgDistrib_Publish(out_pub, &gimbal_out);
    MsgDistrib_Publish(ui_pub, &gimbal_ui);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, delay_tick);
  }
}
