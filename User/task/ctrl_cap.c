/**
 * @file ctrl_cap.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 超级电容控制任务
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 */

/* Includes ----------------------------------------------------------------- */
#include "device/referee.h"
#include "module/cap.h"
#include "task/user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */

/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
static CAN_t can;
static Cap_t cap;

#ifdef DEBUG
CAN_CapOutput_t cap_out;
Referee_ForCap_t referee_cap;
Referee_CapUI_t cap_ui;
#else
static CAN_CapOutput_t cap_out;
static Referee_ForCap_t referee_cap;
static Referee_CapUI_t cap_ui;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * \brief 控制电容
 *
 * \param argument 未使用
 */
void Task_Cap(void *argument) {
  UNUSED(argument); /* 未使用argument，消除警告 */

  /* 计算任务运行到指定频率需要等待的tick数 */
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_CTRL_CAP;

  uint32_t tick = osKernelGetTickCount(); /* 控制任务运行频率的计时 */
  while (1) {
#ifdef DEBUG
    /* 记录任务所使用的的栈空间 */
    task_runtime.stack_water_mark.cap = osThreadGetStackSpace(osThreadGetId());
#endif
    tick += delay_tick; /* 计算下一个唤醒时刻 */

    /* 读取裁判系统信息 */
    osMessageQueueGet(task_runtime.msgq.referee.cap, &referee_cap, 0, 0);

    /* 一定时间长度内接收不到电容反馈值，使电容离线 */
    if (osMessageQueueGet(task_runtime.msgq.can.feedback.cap, &can, NULL,
                          500) != osOK) {
      Cap_HandleOffline(&cap, &cap_out, GAME_CHASSIS_MAX_POWER_WO_REF);
      tick += 500; /* 重新计算下一次唤醒时间 */
    } else {
      osKernelLock(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
      /* 根据裁判系统数据计算输出功率 */
      Cap_Update(&cap, &(can.cap));
      Cap_Control(&referee_cap, &cap_out);
      osKernelUnlock();
    }
    /* 将电容输出值发送到CAN */
    osMessageQueueReset(task_runtime.msgq.can.output.cap);
    osMessageQueuePut(task_runtime.msgq.can.output.cap, &cap_out, 0, 0);
    /* 将电容状态发送到Chassis */
    osMessageQueueReset(task_runtime.msgq.cap_info);
    osMessageQueuePut(task_runtime.msgq.cap_info, &(can.cap), 0, 0);

    Cap_PackUi(&cap, &cap_ui);

    osMessageQueueReset(task_runtime.msgq.ui.cap);
    osMessageQueuePut(task_runtime.msgq.ui.cap, &cap_ui, 0, 0);

    osDelayUntil(tick); /* 运行结束，等待下一次唤醒 */
  }
}
