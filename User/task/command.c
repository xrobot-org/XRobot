/*
  命令接收任务。

  接收机器人的控制指令。

  从DR16中接收数据，转换为通用的CMD_RC_t控制信号。
  根据CMD_RC_t计算最终命令CMD_t。
  把计算好的CMD_t细分后放到对应的消息队列中。
  超时未收到则认为是丢控等特殊情况，把CMD_RC_t中的内容置零，
  在后续的CMD_Parse中会根据此内容发现错误，保证机器人不失控。
*/

/* Includes ----------------------------------------------------------------- */
#include <string.h>

#include "device\dr16.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
#ifdef DEBUG
CMD_RC_t rc_raw;
CMD_AI_t ai_raw;
CMD_t cmd;
#else
static CMD_RC_t rc_raw;
static CMD_AI_t ai_raw;
static CMD_t cmd;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * \brief 控制指令接收
 *
 * \param argument 未使用
 */
void Task_Command(void *argument) {
  (void)argument; /* 未使用argument，消除警告 */

  /* 计算任务运行到指定频率，需要延时的时间 */
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_CTRL_COMMAND;

  /* 初始化指令处理 */
  CMD_Init(&cmd, &(task_runtime.cfg.pilot_cfg->param.cmd));
  uint32_t tick = osKernelGetTickCount(); /* 控制任务运行频率的计时 */
  uint32_t wakeup = HAL_GetTick();

  /* 用于计算遥控器数据频率 */
  while (1) {
#ifdef DEBUG
    /* 记录任务所使用的的栈空间 */
    task_runtime.stack_water_mark.command = osThreadGetStackSpace(NULL);
#endif
    tick += delay_tick; /* 计算下一个唤醒时刻 */

    const uint32_t now = HAL_GetTick();

    osMessageQueueGet(task_runtime.msgq.raw_cmd.rc_raw, &rc_raw, 0, 0);
    osMessageQueueGet(task_runtime.msgq.raw_cmd.ai_raw, &ai_raw, 0, 0);

    osKernelLock(); /* 锁住RTOS内核防止控制过程中断，造成错误 */

    /* 控制权交换 */
    CMD_ChechAiControl(&cmd);
    /* 将接收机数据解析为指令数据 */
    if (cmd.ai_control_right) {
      CMD_ParseAi(&ai_raw, &cmd, 1.0f / (float)TASK_FREQ_CTRL_COMMAND);
    } else {
      CMD_ParseRc(&rc_raw, &cmd, 1.0f / (float)TASK_FREQ_CTRL_COMMAND);
    }

    /* 将需要与其他任务分享的数据放到消息队列中 */
    osMessageQueuePut(task_runtime.msgq.cmd.chassis, &(cmd.chassis), 0, 0);
    osMessageQueuePut(task_runtime.msgq.cmd.gimbal, &(cmd.gimbal), 0, 0);
    osMessageQueuePut(task_runtime.msgq.cmd.shoot, &(cmd.shoot), 0, 0);

    wakeup = now;
    osKernelUnlock(); /* 锁住RTOS内核防止控制过程中断，造成错误 */

    osDelayUntil(tick); /* 运行结束，等待下一次唤醒 */
  }
}
