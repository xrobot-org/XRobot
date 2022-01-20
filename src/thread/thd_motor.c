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

void thd_motor(void* arg) {
  runtime_t* runtime = arg;

  motor_t motor;
  motor_control_t motor_out;

  publisher_t* chassis_fb_pub =
      msg_dist_create_topic("chassis_motor_fb", sizeof(motor_feedback_group_t));
  publisher_t* gimbal_pit_fb_pub = msg_dist_create_topic(
      "gimbal_pit_motor_fb", sizeof(motor_feedback_group_t));
  publisher_t* gimbal_yaw_fb_pub = msg_dist_create_topic(
      "gimbal_yaw_motor_fb", sizeof(motor_feedback_group_t));
  publisher_t* launcher_fric_fb_pub = msg_dist_create_topic(
      "launcher_fric_motor_fb", sizeof(motor_feedback_group_t));
  publisher_t* launcher_trig_fb_pub = msg_dist_create_topic(
      "launcher_trig_motor_fb", sizeof(motor_feedback_group_t));

  subscriber_t* chassis_out_sub = msg_dist_subscribe("chassis_out", true);
  subscriber_t* gimbal_yaw_out_sub = msg_dist_subscribe("gimbal_yaw_out", true);
  subscriber_t* gimbal_pit_out_sub = msg_dist_subscribe("gimbal_pit_out", true);
  subscriber_t* launcher_fric_out_sub =
      msg_dist_subscribe("launcher_fric_out", true);
  subscriber_t* launcher_trig_out_sub =
      msg_dist_subscribe("launcher_trig_out", true);

  motor_init(&motor, runtime->cfg.robot_param->motor);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取裁判系统信息 */
    if (motor_update(&motor, THD_PERIOD_MS)) {
      /* 一定时间长度内接收不到电容反馈值，使电容离线 */
      motor_handle_offline(&motor);
    }

    msg_dist_publish(chassis_fb_pub, motor.feedback + MOTOR_GROUP_ID_CHASSIS);
    msg_dist_publish(gimbal_yaw_fb_pub,
                     motor.feedback + MOTOR_GROUP_ID_GIMBAL_YAW);
    msg_dist_publish(gimbal_pit_fb_pub,
                     motor.feedback + MOTOR_GROUP_ID_GIMBAL_PIT);
    msg_dist_publish(launcher_fric_fb_pub,
                     motor.feedback + MOTOR_GROUP_ID_LAUNCHER_FRIC);
    msg_dist_publish(launcher_trig_fb_pub,
                     motor.feedback + MOTOR_GROUP_ID_LAUNCHER_TRIG);

    if (msg_dist_poll(chassis_out_sub, &motor_out, 0)) {
      motor_pack_data(&motor, MOTOR_GROUP_ID_CHASSIS, &motor_out);
    }

    if (msg_dist_poll(gimbal_yaw_out_sub, &motor_out, 0)) {
      motor_pack_data(&motor, MOTOR_GROUP_ID_GIMBAL_YAW, &motor_out);
    }

    if (msg_dist_poll(gimbal_pit_out_sub, &motor_out, 0)) {
      motor_pack_data(&motor, MOTOR_GROUP_ID_GIMBAL_PIT, &motor_out);
    }

    if (msg_dist_poll(launcher_fric_out_sub, &motor_out, 0)) {
      motor_pack_data(&motor, MOTOR_GROUP_ID_LAUNCHER_FRIC, &motor_out);
    }

    if (msg_dist_poll(launcher_trig_out_sub, &motor_out, 0)) {
      motor_pack_data(&motor, MOTOR_GROUP_ID_LAUNCHER_TRIG, &motor_out);
    }

    motor_control(&motor);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_motor, 256, 3);
