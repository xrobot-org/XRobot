/*
  底盘控制任务

  控制底盘行为。

  从CAN总线接收底盘电机反馈，根据接收到的控制命令，控制电机输出。
*/

/* Includes ----------------------------------------------------------------- */
#include "module\chassis.h"
#include "module\config.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
static CAN_t can;

#ifdef DEBUG
CMD_ChassisCmd_t chassis_cmd;
Chassis_t chassis;
CAN_ChassisOutput_t chassis_out;
#else
static CMD_ChassisCmd_t chassis_cmd;
static Chassis_t chassis;
static CAN_ChassisOutput_t chassis_out;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/*!
 * \brief 控制底盘
 *
 * \param argument 未使用
 */
void Task_CtrlChassis(void *argument) {
  (void)argument; /* 未使用argument，消除警告 */

  /* 计算任务运行到指定频率，需要延时的时间 */
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_CTRL_CHASSIS;

  /* 初始化底盘 */
  Chassis_Init(&chassis, &(task_runtime.robot_param->chassis),
               &task_runtime.robot_cfg.mech_zero,
               (float)TASK_FREQ_CTRL_CHASSIS);

  /* 延时一段时间再开启任务 */
  osMessageQueueGet(task_runtime.msgq.motor.feedback.chassis,
                    &can.chassis_motor, NULL, osWaitForever);

  uint32_t tick = osKernelGetTickCount(); /* 控制任务运行频率的计时 */
  uint32_t wakeup = HAL_GetTick(); /* 计算任务运行间隔的计时 */
  while (1) {
#ifdef DEBUG
    /* 记录任务所使用的的栈空间 */
    task_runtime.stack_water_mark.ctrl_chassis = osThreadGetStackSpace(NULL);
#endif
    tick += delay_tick; /* 计算下一个唤醒时刻 */

    /* 等待接收CAN总线新数据 */
    if (osMessageQueueGet(task_runtime.msgq.motor.feedback.chassis, &can, NULL,
                          delay_tick) != osOK) {
      /* 如果没有接收到新数据，则将输出置零，不进行控制 */
      CAN_ResetChassisOut(&chassis_out);
      osMessageQueuePut(task_runtime.msgq.motor.output.chassis, &chassis_out, 0,
                        0);
    } else {
      /* 继续读取控制指令 */
      osMessageQueueGet(task_runtime.msgq.cmd.chassis, &chassis_cmd, NULL, 0);

      osKernelLock(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
      const uint32_t now = HAL_GetTick();
      Chassis_UpdateFeedback(&chassis, &can);
      Chassis_Control(&chassis, &chassis_cmd, (float)(now - wakeup) / 1000.0f);
      Chassis_DumpOutput(&chassis, &chassis_out);
      osMessageQueuePut(task_runtime.msgq.motor.output.chassis, &chassis_out, 0,
                        0);
      wakeup = now;
      osKernelUnlock();

      osDelayUntil(tick); /* 运行结束，等待下一次唤醒 */
    }
  }
}
