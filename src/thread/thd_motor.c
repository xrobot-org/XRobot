/**
 * @file motor.c
 * @author Qu Shen (503578404@qq.com)
 * @brief Motor总线数据处理
 * @version 1.0.0
 * @date 2021-04-14
 *
 * @copyright Copyright (c) 2021
 *
 * 从消息队列中得到Motor总线收到的原始数据
 * 解析后分组存放、发布
 *
 */

#include "dev_referee.h"
#include "mid_msg_dist.h"
#include "thd.h"

#define THD_PERIOD_MS (2)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void Thd_Motor(void* arg) {
  Runtime_t* runtime = arg;

  Motor_t motor;
  Motor_Control_t motor_out;

  MsgDist_Publisher_t* chassis_fb_pub =
      MsgDist_CreateTopic("chassis_motor_fb", sizeof(Motor_FeedbackGroup_t));
  MsgDist_Publisher_t* gimbal_fb_pub =
      MsgDist_CreateTopic("gimbal_motor_fb", sizeof(Motor_FeedbackGroup_t));
  MsgDist_Publisher_t* launcher_fb_pub =
      MsgDist_CreateTopic("launcher_motor_fb", sizeof(Motor_Control_t));

  MsgDist_Subscriber_t* chassis_out_sub =
      MsgDist_Subscribe("chassis_out", true);
  MsgDist_Subscriber_t* gimbal_out_sub = MsgDist_Subscribe("gimbal_out", true);
  MsgDist_Subscriber_t* launcher_out_sub =
      MsgDist_Subscribe("launcher_out", true);

  Motor_Init(&motor, runtime->cfg.robot_param->motor);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取裁判系统信息 */
    if (Motor_Update(&motor, THD_PERIOD_MS)) {
      /* 一定时间长度内接收不到电容反馈值，使电容离线 */
      Motor_HandleOffline(&motor);
    }

    MsgDist_Publish(chassis_fb_pub, motor.feedback + MOTOR_GROUT_ID_CHASSIS);
    MsgDist_Publish(gimbal_fb_pub, motor.feedback + MOTOR_GROUT_ID_GIMBAL1);
    MsgDist_Publish(launcher_fb_pub, motor.feedback + MOTOR_GROUT_ID_LAUNCHER1);

    if (MsgDist_Poll(chassis_out_sub, &motor_out, 0)) {
      Motor_Control(&motor, MOTOR_GROUT_ID_CHASSIS, &motor_out);
    }

    if (MsgDist_Poll(gimbal_out_sub, &motor_out, 0)) {
      Motor_Control(&motor, MOTOR_GROUT_ID_GIMBAL1, &motor_out);
    }

    if (MsgDist_Poll(launcher_out_sub, &motor_out, 0)) {
      Motor_Control(&motor, MOTOR_GROUT_ID_LAUNCHER1, &motor_out);
    }

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
