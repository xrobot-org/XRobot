/**
 * @file ctrl_launcher.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 发射器控制线程
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "mid_msg_dist.h"
#include "mod_launcher.h"
#include "thd.h"

#define THD_PERIOD_MS (2)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_ctrl_launcher(void* arg) {
  runtime_t* runtime = arg;

  launcher_t launcher;
  cmd_launcher_t launcher_cmd;
  motor_feedback_group_t launcher_motor_fb;
  referee_for_launcher_t referee_launcher;
  motor_control_t launcher_out;
  ui_launcher_t launcher_ui;

  publisher_t* out_pub =
      msg_dist_create_topic("launcher_out", sizeof(motor_control_t));
  publisher_t* ui_pub =
      msg_dist_create_topic("launcher_ui", sizeof(ui_gimbal_t));

  subscriber_t* motor_sub = msg_dist_subscribe("launcher_motor_fb", true);
  subscriber_t* ref_sub = msg_dist_subscribe("referee_launcher", true);
  subscriber_t* cmd_sub = msg_dist_subscribe("cmd_launcher", true);

  /* 初始化发射器 */
  launcher_init(&launcher, &(runtime->cfg.robot_param->launcher),
                1000.0f / (float)THD_PERIOD_MS);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取控制指令、姿态、IMU、裁判系统、电机反馈 */
    msg_dist_poll(motor_sub, &launcher_motor_fb, 0);
    msg_dist_poll(ref_sub, &referee_launcher, 0);
    msg_dist_poll(cmd_sub, &launcher_cmd, 0);

    vTaskSuspendAll(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
    launcher_update_feedback(&launcher, &launcher_motor_fb);
    launcher_control(&launcher, &launcher_cmd, &referee_launcher,
                     xTaskGetTickCount());
    launcher_pack_output(&launcher, &launcher_out);
    launcher_pack_ui(&launcher, &launcher_ui);
    xTaskResumeAll();

    msg_dist_publish(out_pub, &launcher_out);
    msg_dist_publish(ui_pub, &launcher_ui);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_ctrl_launcher, 256, 2);
