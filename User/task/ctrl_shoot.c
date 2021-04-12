/*
  射击控制任务

  控制射击行为。

  从CAN总线接收底盘电机反馈，根据接收到的控制命令，控制电机输出。
*/

/* Includes ----------------------------------------------------------------- */
#include "module\shoot.h"
#include "task/user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
static CAN_t can;

#ifdef DEBUG
CMD_ShootCmd_t shoot_cmd;
Shoot_t shoot;
Referee_ForShoot_t referee_shoot;
CAN_ShootOutput_t shoot_out;
Referee_ShootUI_t shoot_ui;
#else
static CMD_ShootCmd_t shoot_cmd;
static Shoot_t shoot;
static Referee_ForShoot_t referee_shoot;
static CAN_ShootOutput_t shoot_out;
static Referee_ShootUI_t shoot_ui;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * \brief 控制射击
 *
 * \param argument 未使用
 */
void Task_CtrlShoot(void *argument) {
  (void)argument; /* 未使用argument，消除警告 */

  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_CTRL_SHOOT;
  /* 初始化射击 */
  Shoot_Init(&shoot, &(task_runtime.cfg.robot_param->shoot),
             (float)TASK_FREQ_CTRL_SHOOT);

  /* 延时一段时间再开启任务 */
  osMessageQueueGet(task_runtime.msgq.can.feedback.shoot, &can, NULL,
                    osWaitForever);
  uint32_t tick = osKernelGetTickCount(); /* 控制任务运行频率的计时 */
  while (1) {
#ifdef DEBUG
    /* 记录任务所使用的的栈空间 */
    task_runtime.stack_water_mark.ctrl_shoot =
        osThreadGetStackSpace(osThreadGetId());
#endif
    if (osMessageQueueGet(task_runtime.msgq.can.feedback.shoot, &can, NULL,
                          0) != osOK) {
      // Error handler
      Shoot_ResetOutput(&shoot_out);
    } else {
      /* 读取控制指令以及裁判系统信息 */
      osMessageQueueGet(task_runtime.msgq.cmd.shoot, &shoot_cmd, NULL, 0);
      osMessageQueueGet(task_runtime.msgq.referee.shoot, &referee_shoot, NULL,
                        0);
      osKernelLock(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
      Shoot_UpdateFeedback(&shoot, &can);
      /* 根据指令控制射击 */
      Shoot_Control(&shoot, &shoot_cmd, &referee_shoot, HAL_GetTick());
      /* 复制射击输出值 */
      Shoot_DumpOutput(&shoot, &shoot_out);
      osKernelUnlock();
    }
    osMessageQueueReset(task_runtime.msgq.can.output.shoot);
    osMessageQueuePut(task_runtime.msgq.can.output.shoot, &shoot_out, 0, 0);

    Shoot_DumpUI(&shoot, &shoot_ui);
    osMessageQueueReset(task_runtime.msgq.ui.shoot);
    osMessageQueuePut(task_runtime.msgq.ui.shoot, &shoot_ui, 0, 0);

    tick += delay_tick; /* 计算下一个唤醒时刻 */
    osDelayUntil(tick); /* 运行结束，等待下一次唤醒 */
  }
}
