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

#ifdef DEBUG
CAN_t can;
CMD_ChassisCtrl_t chassis_ctrl;
Chassis_t chassis;
#else
static CAN_t can;
static CMD_ChassisCtrl_t chassis_ctrl;
static Chassis_t chassis;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */
void Task_CtrlChassis(void *argument) {
  (void)argument;
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_CTRL_CHASSIS;

  /* Device Setup */
  osDelay(TASK_INIT_DELAY_CTRL_CHASSIS);

  osThreadId_t recv_motor_allert[3] = {osThreadGetId(),
                                       task_runtime.thread.ctrl_gimbal,
                                       task_runtime.thread.ctrl_shoot};

  CAN_Init(&can, NULL, recv_motor_allert, 3, task_runtime.thread.referee);

  /* Module Setup */
  Chassis_Init(&chassis, &(task_runtime.config_robot->param.chassis),
               (float)TASK_FREQ_HZ_CTRL_CHASSIS);

  /* Task Setup */
  uint32_t tick = osKernelGetTickCount();
  uint32_t wakeup = HAL_GetTick();
  while (1) {
#ifdef DEBUG
    task_runtime.stack_water_mark.ctrl_chassis = osThreadGetStackSpace(NULL);
#endif
    /* Task body */
    tick += delay_tick;

    const uint32_t flag = SIGNAL_CAN_MOTOR_RECV;
    if (osThreadFlagsWait(flag, osFlagsWaitAll, delay_tick) != flag) {
      CAN_Motor_ControlChassis(0.0f, 0.0f, 0.0f, 0.0f);

    } else {
      osMessageQueueGet(task_runtime.msgq.cmd.chassis, &chassis_ctrl, NULL, 0);

      osKernelLock();
      const uint32_t now = HAL_GetTick();
      Chassis_UpdateFeedback(&chassis, &can);
      Chassis_Control(&chassis, &chassis_ctrl, (float)(now - wakeup) / 1000.0f);
      wakeup = now;
      CAN_Motor_ControlChassis(chassis.out[0], chassis.out[1], chassis.out[2],
                               chassis.out[3]);

      osKernelUnlock();

      osDelayUntil(tick);
    }
  }
}
