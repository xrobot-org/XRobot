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

#include "dev_referee.hpp"
#include "om.h"
#include "thd.hpp"

#define THD_PERIOD_MS (2)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_motor(void* arg) {
  runtime_t* runtime = (runtime_t*)arg;

  motor_t motor;
  motor_control_t motor_out;

  om_topic_t* chassis_fb_tp = om_config_topic(NULL, "A", "chassis_motor_fb",
                                              sizeof(motor_feedback_group_t));
  om_topic_t* gimbal_pit_fb_tp =
      om_config_topic(NULL, "A", "gimbal_pit_motor_fb");
  om_topic_t* gimbal_yaw_fb_tp =
      om_config_topic(NULL, "A", "gimbal_yaw_motor_fb");
  om_topic_t* launcher_fric_fb_tp =
      om_config_topic(NULL, "A", "launcher_fric_motor_fb");
  om_topic_t* launcher_trig_fb_tp =
      om_config_topic(NULL, "A", "launcher_trig_motor_fb");
  om_topic_t* chassis_out_tp = om_find_topic("chassis_out", UINT32_MAX);
  om_topic_t* gimbal_yaw_out_tp = om_find_topic("gimbal_yaw_out", UINT32_MAX);
  om_topic_t* gimbal_pit_out_tp = om_find_topic("gimbal_pit_out", UINT32_MAX);
  om_topic_t* la_fric_out_tp = om_find_topic("launcher_fric_out", UINT32_MAX);
  om_topic_t* la_trig_out_tp = om_find_topic("launcher_trig_out", UINT32_MAX);

  om_suber_t* chassis_out_sub =
      om_subscript(chassis_out_tp, OM_PRASE_VAR(motor_out));
  om_suber_t* gimbal_yaw_out_sub =
      om_subscript(gimbal_yaw_out_tp, OM_PRASE_VAR(motor_out));
  om_suber_t* gimbal_pit_out_sub =
      om_subscript(gimbal_pit_out_tp, OM_PRASE_VAR(motor_out));
  om_suber_t* launcher_fric_out_sub =
      om_subscript(la_fric_out_tp, OM_PRASE_VAR(motor_out));
  om_suber_t* launcher_trig_out_sub =
      om_subscript(la_trig_out_tp, OM_PRASE_VAR(motor_out));

  motor_init(&motor, runtime->cfg.robot_param->motor);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取裁判系统信息 */
    if (motor_update(&motor)) {
      /* 一定时间长度内接收不到电容反馈值，使电容离线 */
      motor_handle_offline(&motor);
    }

    om_publish(chassis_fb_tp, motor.feedback + MOTOR_GROUP_ID_CHASSIS,
               sizeof(*motor.feedback), true, false);
    om_publish(gimbal_yaw_fb_tp, motor.feedback + MOTOR_GROUP_ID_GIMBAL_YAW,
               sizeof(*motor.feedback), true, false);
    om_publish(gimbal_pit_fb_tp, motor.feedback + MOTOR_GROUP_ID_GIMBAL_PIT,
               sizeof(*motor.feedback), true, false);
    om_publish(launcher_fric_fb_tp,
               motor.feedback + MOTOR_GROUP_ID_LAUNCHER_FRIC,
               sizeof(*motor.feedback), true, false);
    om_publish(launcher_trig_fb_tp,
               motor.feedback + MOTOR_GROUP_ID_LAUNCHER_TRIG,
               sizeof(*motor.feedback), true, false);

    if (om_suber_export(chassis_out_sub, false) == OM_OK) {
      motor_pack_data(&motor, MOTOR_GROUP_ID_CHASSIS, &motor_out);
    }

    if (om_suber_export(gimbal_yaw_out_sub, false) == OM_OK) {
      motor_pack_data(&motor, MOTOR_GROUP_ID_GIMBAL_YAW, &motor_out);
    }

    if (om_suber_export(gimbal_pit_out_sub, false) == OM_OK) {
      motor_pack_data(&motor, MOTOR_GROUP_ID_GIMBAL_PIT, &motor_out);
    }

    if (om_suber_export(launcher_fric_out_sub, false) == OM_OK) {
      motor_pack_data(&motor, MOTOR_GROUP_ID_LAUNCHER_FRIC, &motor_out);
    }

    if (om_suber_export(launcher_trig_out_sub, false) == OM_OK) {
      motor_pack_data(&motor, MOTOR_GROUP_ID_LAUNCHER_TRIG, &motor_out);
    }

    motor_control(&motor);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_motor, 384, 3);
