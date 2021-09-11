/**
 * @file can.c
 * @author Qu Shen (503578404@qq.com)
 * @brief CAN总线数据处理
 * @version 1.0.0
 * @date 2021-04-14
 *
 * @copyright Copyright (c) 2021
 *
 * 从消息队列中得到CAN总线收到的原始数据
 * 解析后分组存放、发布
 *
 */

#include "dev_can.h"
#include "dev_referee.h"
#include "mid_msg_distrib.h"
#include "thd.h"

#ifdef MCU_DEBUG_BUILD

CAN_t can;
CAN_Output_t can_out;
CAN_RawRx_t can_rx;

#else

static CAN_t can;
static CAN_Output_t can_out;
static CAN_RawRx_t can_rx;

#endif

#define THD_PERIOD_MS (2)

void Thd_CAN(void* argument) {
  Runtime_t* runtime = argument;
  const uint32_t delay_tick = pdMS_TO_TICKS(THD_PERIOD_MS);

  MsgDistrib_Publisher_t* chassis_fb_pub =
      MsgDistrib_CreateTopic("chassis_motor_fb", sizeof(CAN_ChassisMotor_t));
  MsgDistrib_Publisher_t* gimbal_fb_pub =
      MsgDistrib_CreateTopic("gimbal_motor_fb", sizeof(CAN_GimbalMotor_t));
  MsgDistrib_Publisher_t* launcher_fb_pub =
      MsgDistrib_CreateTopic("launcher_motor_fb", sizeof(CAN_LauncherMotor_t));
  MsgDistrib_Publisher_t* cap_fb_pub =
      MsgDistrib_CreateTopic("cap_fb", sizeof(CAN_CapFeedback_t));
  MsgDistrib_Publisher_t* tof_fb_pub =
      MsgDistrib_CreateTopic("tof_fb", sizeof(CAN_Tof_t));

  MsgDistrib_Subscriber_t* chassis_out_sub =
      MsgDistrib_Subscribe("chassis_out", true);
  MsgDistrib_Subscriber_t* gimbal_out_sub =
      MsgDistrib_Subscribe("gimbal_out", true);
  MsgDistrib_Subscriber_t* launcher_out_sub =
      MsgDistrib_Subscribe("launcher_out", true);
  MsgDistrib_Subscriber_t* cap_out_sub = MsgDistrib_Subscribe("cap_out", true);

  CAN_Init(&can, &runtime->cfg.robot_param->can);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    while (xQueueReceive(can.msgq_raw, &can_rx, 0) == pdPASS) {
      CAN_StoreMsg(&can, &can_rx);
    }

    MsgDistrib_Publish(chassis_fb_pub, &(can.motor.chassis));
    MsgDistrib_Publish(gimbal_fb_pub, &(can.motor.gimbal));
    MsgDistrib_Publish(launcher_fb_pub, &(can.motor.launcher));

    if (CAN_CheckFlag(&can, CAN_REC_CAP_FINISHED, true)) {
      MsgDistrib_Publish(cap_fb_pub, &(can.cap));
    }

    if (CAN_CheckFlag(&can, CAN_REC_TOF_FINISHED, true)) {
      MsgDistrib_Publish(tof_fb_pub, &(can.cap));
    }

    if (MsgDistrib_Poll(chassis_out_sub, &(can_out.chassis), 0)) {
      CAN_Motor_Control(CAN_MOTOR_GROUT_CHASSIS, &can_out, &can);
    }

    if (MsgDistrib_Poll(gimbal_out_sub, &(can_out.gimbal), 0)) {
      CAN_Motor_Control(CAN_MOTOR_GROUT_GIMBAL1, &can_out, &can);
    }

    if (MsgDistrib_Poll(launcher_out_sub, &(can_out.launcher), 0)) {
      CAN_Motor_Control(CAN_MOTOR_GROUT_LAUNCHER1, &can_out, &can);
    }

    if (MsgDistrib_Poll(cap_out_sub, &(can_out.cap), 0)) {
      CAN_Cap_Control(&(can_out.cap), &can);
    }

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, delay_tick);
  }
}
