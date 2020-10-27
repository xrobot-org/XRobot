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
static CAN_t *can;

#ifdef DEBUG
CMD_ShootCmd_t shoot_cmd;
Shoot_t shoot;
#else
static CMD_ShootCmd_t shoot_cmd;
static Shoot_t shoot;
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

  osDelay(TASK_INIT_DELAY_CTRL_SHOOT); /* 延时一段时间再开启任务 */

  /* 等待CAN总线设备初始化完成 */
  while ((can = CAN_GetDevice()) == NULL) {
    osDelay(delay_tick);
  }

  /* 初始化射击 */
  Shoot_Init(&shoot, &(task_runtime.robot_param->shoot),
             (float)TASK_FREQ_CTRL_SHOOT);

  uint32_t tick = osKernelGetTickCount(); /* 控制任务运行频率的计时 */
  uint32_t wakeup = HAL_GetTick(); /* 计算任务运行间隔的计时 */
  while (1) {
#ifdef DEBUG
    /* 记录任务所使用的的栈空间 */
    task_runtime.stack_water_mark.ctrl_shoot = osThreadGetStackSpace(NULL);
#endif
    tick += delay_tick; /* 计算下一个唤醒时刻 */

    /* 等待接收CAN总线新数据 */
    const uint32_t flag = SIGNAL_CAN_MOTOR_RECV;
    if (osThreadFlagsWait(flag, osFlagsWaitAll, delay_tick) != flag) {
      /* 如果没有接收到新数据，则将输出置零，不进行控制 */
      CAN_Motor_ControlShoot(0.0f, 0.0f, 0.0f);

    } else {
      /* 继续读取控制指令 */
      osMessageQueueGet(task_runtime.msgq.cmd.shoot, &shoot_cmd, NULL, 0);

      osKernelLock(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
      const uint32_t now = HAL_GetTick();
      Shoot_UpdateFeedback(&shoot, can);
      Shoot_Control(&shoot, &shoot_cmd, (float)(now - wakeup) / 1000.0f);
      wakeup = now;
      CAN_Motor_ControlShoot(shoot.out[SHOOT_ACTR_FRIC1_IDX],
                             shoot.out[SHOOT_ACTR_FRIC2_IDX],
                             shoot.out[SHOOT_ACTR_TRIG_IDX]);
      osKernelUnlock();

      osDelayUntil(tick); /* 运行结束，等待下一次唤醒 */
    }
  }
}
