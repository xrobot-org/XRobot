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
#include "mid_msg_distrib.h"
#include "mod_chassis.h"
#include "mod_config.h"
#include "thd.h"

static Cap_t cap;

#ifdef MCU_DEBUG_BUILD

Chassis_t chassis;
CMD_ChassisCmd_t chassis_cmd;
CAN_ChassisMotor_t chassis_motor;
CAN_GimbalMotor_t gimbal_motor;
Referee_ForChassis_t chassis_ref;
CAN_ChassisOutput_t chassis_out;
UI_ChassisUI_t chassis_ui;

#else

static Chassis_t chassis;
static CMD_ChassisCmd_t chassis_cmd;
CAN_ChassisMotor_t chassis_motor;
static Referee_ForChassis_t chassis_ref;
static CAN_ChassisOutput_t chassis_out;
static UI_ChassisUI_t chassis_ui;

#endif

#define THD_PERIOD_MS (2)

void Thd_CtrlChassis(void* arg) {
  Runtime_t* runtime = arg;
  const uint32_t delay_tick = pdMS_TO_TICKS(THD_PERIOD_MS);

  MsgDistrib_Publisher_t* out_pub =
      MsgDistrib_CreateTopic("chassis_out", sizeof(CAN_ChassisOutput_t));
  MsgDistrib_Publisher_t* ui_pub =
      MsgDistrib_CreateTopic("chassis_ui", sizeof(UI_ChassisUI_t));

  MsgDistrib_Subscriber_t* ref_sub =
      MsgDistrib_Subscribe("referee_chassis", true);
  MsgDistrib_Subscriber_t* cap_sub = MsgDistrib_Subscribe("chassis_gyro", true);
  MsgDistrib_Subscriber_t* chassis_motor_sub =
      MsgDistrib_Subscribe("chassis_motor_fb", true);
  MsgDistrib_Subscriber_t* gimbal_motor_sub =
      MsgDistrib_Subscribe("gimbal_motor_fb", true);
  MsgDistrib_Subscriber_t* cmd_sub = MsgDistrib_Subscribe("cmd_chassis", true);

  /* 初始化底盘 */
  Chassis_Init(&chassis, &(runtime->cfg.robot_param->chassis),
               &(runtime->cfg.gimbal_mech_zero),
               1000.0f / (float)THD_PERIOD_MS);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取控制指令、电容、裁判系统、电机反馈 */
    MsgDistrib_Poll(chassis_motor_sub, &chassis_motor, 0);
    MsgDistrib_Poll(gimbal_motor_sub, &gimbal_motor, 0);
    MsgDistrib_Poll(ref_sub, &chassis_ref, 0);
    MsgDistrib_Poll(cmd_sub, &chassis_cmd, 0);
    MsgDistrib_Poll(cap_sub, &cap, 0);

    vTaskSuspendAll(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
    /* 更新反馈值 */
    Chassis_UpdateFeedback(&chassis, &chassis_motor, &gimbal_motor);
    Chassis_Control(&chassis, &chassis_cmd, xTaskGetTickCount());
    Chassis_PowerLimit(&chassis, &cap, &chassis_ref); /* 限制输出功率 */
    Chassis_PackOutput(&chassis, &chassis_out);
    Chassis_PackUi(&chassis, &chassis_ui);
    xTaskResumeAll();

    MsgDistrib_Publish(out_pub, &chassis_out);
    MsgDistrib_Publish(ui_pub, &chassis_ui);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, delay_tick);
  }
}
