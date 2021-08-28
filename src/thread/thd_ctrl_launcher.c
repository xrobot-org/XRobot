/**
 * @file ctrl_launcher.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 发射器控制任务
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 * 通过消息队列收集发射器控制需要电机反馈
 * 运行launcher模组
 * 通过消息队列发送发射器控制输出的数据
 *
 */

/* Includes ----------------------------------------------------------------- */
#include "mod_launcher.h"
#include "thd.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
static CAN_t can;

#ifdef MCU_DEBUG_BUILD
CMD_LauncherCmd_t launcher_cmd;
Launcher_t launcher;
Referee_ForLauncher_t referee_launcher;
CAN_LauncherOutput_t launcher_out;
UI_LauncherUI_t launcher_ui;
#else
static CMD_LauncherCmd_t launcher_cmd;
static Launcher_t launcher;
static Referee_ForLauncher_t referee_launcher;
static CAN_LauncherOutput_t launcher_out;
static UI_LauncherUI_t launcher_ui;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * @brief 控制发射器
 *
 * @param argument 未使用
 */
void Thread_CtrlLauncher(void *argument) {
  RM_UNUSED(argument); /* 未使用argument，消除警告 */

  const uint32_t delay_tick = pdMS_TO_TICKS(1000 / TASK_FREQ_CTRL_LAUNCHER);
  /* 初始化发射器 */
  Launcher_Init(&launcher, &(runtime.cfg.robot_param->launcher),
                (float)TASK_FREQ_CTRL_LAUNCHER);

  /* 延时一段时间再开启任务 */
  xQueueReceive(runtime.msgq.can.feedback.launcher, &can, portMAX_DELAY);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    xQueueReceive(runtime.msgq.can.feedback.launcher, &can, 0);
    /* 读取控制指令以及裁判系统信息 */
    xQueueReceive(runtime.msgq.cmd.launcher, &launcher_cmd, 0);
    xQueueReceive(runtime.msgq.referee.launcher, &referee_launcher, 0);

    vTaskSuspendAll(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
    Launcher_UpdateFeedback(&launcher, &can);
    Launcher_Control(&launcher, &launcher_cmd, &referee_launcher,
                     xTaskGetTickCount());
    Launcher_PackOutput(&launcher, &launcher_out);
    Launcher_PackUi(&launcher, &launcher_ui);
    xTaskResumeAll();

    xQueueOverwrite(runtime.msgq.can.output.launcher, &launcher_out);
    xQueueOverwrite(runtime.msgq.ui.launcher, &launcher_ui);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, delay_tick);
  }
}
