/*
  射击控制任务

  控制射击行为。

  从CAN总线接收底盘电机反馈，根据接收到的控制命令，控制电机输出。
*/

/* Includes ----------------------------------------------------------------- */
#include "module\config.h"
#include "module\shoot.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
static CAN_t can;

#ifdef DEBUG
CMD_ShootCmd_t shoot_cmd;
Shoot_t shoot;
CAN_ShootOutput_t shoot_out;
#else
static CMD_ShootCmd_t shoot_cmd;
static Shoot_t shoot;
static CAN_ShootOutput_t shoot_out;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/*!
 * \brief 控制射击
 *
 * \param argument 未使用
 */
void Task_CtrlShoot(void *argument) {
  (void)argument; /* 未使用argument，消除警告 */

  /* 计算任务运行到指定频率，需要延时的时间 */
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_CTRL_SHOOT;

  // osDelay(TASK_INIT_DELAY_CTRL_SHOOT); /* 延时一段时间再开启任务 */

  /* 初始化射击 */
  Shoot_Init(&shoot, &(task_runtime.robot_param->shoot),
             (float)TASK_FREQ_CTRL_SHOOT);

  osMessageQueueGet(task_runtime.msgq.motor.feedback.shoot, &can, NULL,
                    delay_tick);

  uint32_t tick = osKernelGetTickCount(); /* 控制任务运行频率的计时 */
  uint32_t wakeup = HAL_GetTick(); /* 计算任务运行间隔的计时 */
  while (1) {
#ifdef DEBUG
    /* 记录任务所使用的的栈空间 */
    task_runtime.stack_water_mark.ctrl_shoot = osThreadGetStackSpace(NULL);
#endif
    tick += delay_tick; /* 计算下一个唤醒时刻 */

    /* 等待接收CAN总线新数据 */
    if (osMessageQueueGet(task_runtime.msgq.motor.feedback.shoot,
                          &(can.shoot_motor), NULL, delay_tick) != osOK) {
      /* 如果没有接收到新数据，则将输出置零，不进行控制 */
      CAN_ResetShootOut(&shoot_out);
      osMessageQueuePut(task_runtime.msgq.motor.output.shoot, &shoot_out, 0, 0);
    } else {
      /* 继续读取控制指令 */
      osMessageQueueGet(task_runtime.msgq.cmd.shoot, &shoot_cmd, NULL, 0);

      osKernelLock(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
      const uint32_t now = HAL_GetTick();
      Shoot_UpdateFeedback(&shoot, &can);
      Shoot_Control(&shoot, &shoot_cmd, (float)(now - wakeup) / 1000.0f);
      Shoot_DumpOutput(&shoot, &shoot_out);
      osMessageQueuePut(task_runtime.msgq.motor.output.shoot, &shoot_out, 0, 0);
      wakeup = now;

      osKernelUnlock();

      osDelayUntil(tick); /* 运行结束，等待下一次唤醒 */
    }
  }
}
