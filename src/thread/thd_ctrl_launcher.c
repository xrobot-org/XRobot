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

void Thd_CtrlLauncher(void* arg) {
  Runtime_t* runtime = arg;

  Launcher_t launcher;
  CMD_LauncherCmd_t launcher_cmd;
  Motor_FeedbackGroup_t launcher_motor_fb;
  Referee_ForLauncher_t referee_launcher;
  Motor_Control_t launcher_out;
  UI_LauncherUI_t launcher_ui;

  MsgDist_Publisher_t* out_pub =
      MsgDist_CreateTopic("launcher_out", sizeof(Motor_Control_t));
  MsgDist_Publisher_t* ui_pub =
      MsgDist_CreateTopic("launcher_ui", sizeof(UI_GimbalUI_t));

  MsgDist_Subscriber_t* motor_sub =
      MsgDist_Subscribe("launcher_motor_fb", true);
  MsgDist_Subscriber_t* ref_sub = MsgDist_Subscribe("launcher_eulr", true);
  MsgDist_Subscriber_t* cmd_sub = MsgDist_Subscribe("cmd_launcher", true);

  /* 初始化发射器 */
  Launcher_Init(&launcher, &(runtime->cfg.robot_param->launcher),
                1000.0f / (float)THD_PERIOD_MS);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取控制指令、姿态、IMU、裁判系统、电机反馈 */
    MsgDist_Poll(motor_sub, &launcher_motor_fb, 0);
    MsgDist_Poll(ref_sub, &referee_launcher, 0);
    MsgDist_Poll(cmd_sub, &launcher_cmd, 0);

    vTaskSuspendAll(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
    Launcher_UpdateFeedback(&launcher, &launcher_motor_fb);
    Launcher_Control(&launcher, &launcher_cmd, &referee_launcher,
                     xTaskGetTickCount());
    Launcher_PackOutput(&launcher, &launcher_out);
    Launcher_PackUi(&launcher, &launcher_ui);
    xTaskResumeAll();

    MsgDist_Publish(out_pub, &launcher_out);
    MsgDist_Publish(ui_pub, &launcher_ui);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
