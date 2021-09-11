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

#include "mid_msg_distrib.h"
#include "mod_launcher.h"
#include "thd.h"

#ifdef MCU_DEBUG_BUILD

Launcher_t launcher;
CMD_LauncherCmd_t launcher_cmd;
CAN_LauncherMotor_t launcher_motor;
Referee_ForLauncher_t referee_launcher;
CAN_LauncherOutput_t launcher_out;
UI_LauncherUI_t launcher_ui;

#else

static Launcher_t launcher;
static CMD_LauncherCmd_t launcher_cmd;
static CAN_LauncherMotor_t launcher_motor;
static Referee_ForLauncher_t referee_launcher;
static CAN_LauncherOutput_t launcher_out;
static UI_LauncherUI_t launcher_ui;

#endif

#define THD_PERIOD_MS (2)

void Thd_CtrlLauncher(void* argument) {
  Runtime_t* runtime = argument;
  const uint32_t delay_tick = pdMS_TO_TICKS(THD_PERIOD_MS);

  MsgDistrib_Publisher_t* out_pub =
      MsgDistrib_CreateTopic("launcher_out", sizeof(CAN_GimbalOutput_t));
  MsgDistrib_Publisher_t* ui_pub =
      MsgDistrib_CreateTopic("launcher_ui", sizeof(UI_GimbalUI_t));

  MsgDistrib_Subscriber_t* motor_sub =
      MsgDistrib_Subscribe("launcher_motor_fb", true);
  MsgDistrib_Subscriber_t* ref_sub =
      MsgDistrib_Subscribe("launcher_eulr", true);
  MsgDistrib_Subscriber_t* cmd_sub = MsgDistrib_Subscribe("cmd_launcher", true);

  /* 初始化发射器 */
  Launcher_Init(&launcher, &(runtime->cfg.robot_param->launcher),
                1000.0f / (float)THD_PERIOD_MS);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取控制指令、姿态、IMU、裁判系统、电机反馈 */
    MsgDistrib_Poll(motor_sub, &launcher_motor, 0);
    MsgDistrib_Poll(ref_sub, &referee_launcher, 0);
    MsgDistrib_Poll(cmd_sub, &launcher_cmd, 0);

    vTaskSuspendAll(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
    Launcher_UpdateFeedback(&launcher, &launcher_motor);
    Launcher_Control(&launcher, &launcher_cmd, &referee_launcher,
                     xTaskGetTickCount());
    Launcher_PackOutput(&launcher, &launcher_out);
    Launcher_PackUi(&launcher, &launcher_ui);
    xTaskResumeAll();

    MsgDistrib_Publish(out_pub, &launcher_out);
    MsgDistrib_Publish(ui_pub, &launcher_ui);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, delay_tick);
  }
}
