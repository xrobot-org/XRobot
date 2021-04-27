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
#include "module\launcher.h"
#include "task/user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
static CAN_t can;

#ifdef DEBUG
CMD_LauncherCmd_t launcher_cmd;
Launcher_t launcher;
Referee_ForLauncher_t referee_launcher;
CAN_LauncherOutput_t launcher_out;
Referee_LauncherUI_t launcher_ui;
#else
static CMD_LauncherCmd_t launcher_cmd;
static Launcher_t launcher;
static Referee_ForLauncher_t referee_launcher;
static CAN_LauncherOutput_t launcher_out;
static Referee_LauncherUI_t launcher_ui;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * \brief 控制发射器
 *
 * \param argument 未使用
 */
void Task_CtrlLauncher(void *argument) {
  (void)argument; /* 未使用argument，消除警告 */

  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_CTRL_LAUNCHER;
  /* 初始化发射器 */
  Launcher_Init(&launcher, &(task_runtime.cfg.robot_param->launcher),
                (float)TASK_FREQ_CTRL_LAUNCHER);

  /* 延时一段时间再开启任务 */
  osMessageQueueGet(task_runtime.msgq.can.feedback.launcher, &can, NULL,
                    osWaitForever);
  uint32_t tick = osKernelGetTickCount(); /* 控制任务运行频率的计时 */
  while (1) {
#ifdef DEBUG
    /* 记录任务所使用的的栈空间 */
    task_runtime.stack_water_mark.ctrl_launcher =
        osThreadGetStackSpace(osThreadGetId());
#endif
    osMessageQueueGet(task_runtime.msgq.can.feedback.launcher, &can, NULL, 0);
    /* 读取控制指令以及裁判系统信息 */
    osMessageQueueGet(task_runtime.msgq.cmd.launcher, &launcher_cmd, NULL, 0);
    osMessageQueueGet(task_runtime.msgq.referee.launcher, &referee_launcher,
                      NULL, 0);

    osKernelLock(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
    Launcher_UpdateFeedback(&launcher, &can);
    Launcher_Control(&launcher, &launcher_cmd, &referee_launcher, tick);
    Launcher_PackOutput(&launcher, &launcher_out);
    Launcher_PackUi(&launcher, &launcher_ui);
    osKernelUnlock();

    osMessageQueueReset(task_runtime.msgq.can.output.launcher);
    osMessageQueuePut(task_runtime.msgq.can.output.launcher, &launcher_out, 0,
                      0);

    osMessageQueueReset(task_runtime.msgq.ui.launcher);
    osMessageQueuePut(task_runtime.msgq.ui.launcher, &launcher_ui, 0, 0);

    tick += delay_tick; /* 计算下一个唤醒时刻 */
    osDelayUntil(tick); /* 运行结束，等待下一次唤醒 */
  }
}
