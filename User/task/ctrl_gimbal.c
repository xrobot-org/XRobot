/*
        控制云台。

*/

/* Includes ------------------------------------------------------------------*/
#include "module\gimbal.h"
#include "module\robot.h"
#include "task\user_task.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static CAN_t *can;
static BMI088_t *imu;
static CMD_t *cmd;
static Gimbal_t gimbal;

/* Private function ----------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_CtrlGimbal(void *argument) {
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_CTRL_GIMBAL;
  Task_Param_t *task_param = (Task_Param_t *)argument;

  /* Task Setup */
  osDelay(TASK_INIT_DELAY_CTRL_GIMBAL);

  while ((can = CAN_GetDevice()) == NULL) {
    osDelay(delay_tick);
  }

  Gimbal_Init(&gimbal, &(task_param->config_robot->param.gimbal),
              (float)delay_tick / (float)osKernelGetTickFreq(), imu);

  uint32_t tick = osKernelGetTickCount();
  while (1) {
#ifdef DEBUG
    task_param->stack_water_mark.ctrl_gimbal = osThreadGetStackSpace(NULL);
#endif
    /* Task body */
    tick += delay_tick;

    uint32_t flag = SIGNAL_CAN_MOTOR_RECV;
    if (osThreadFlagsWait(flag, osFlagsWaitAll, delay_tick) ==
        osFlagsErrorTimeout) {
      CAN_Motor_ControlGimbal(0.f, 0.f);

    } else {
      osMessageQueueGet(task_param->msgq.gimbal_eulr, gimbal.fb.eulr.imu, NULL, 0);
      osMessageQueueGet(task_param->msgq.cmd, cmd, NULL, 0);

      osKernelLock();
      Gimbal_UpdateFeedback(&gimbal, can);
      Gimbal_Control(&gimbal, &(cmd->gimbal));
      CAN_Motor_ControlGimbal(gimbal.out[GIMBAL_ACTR_YAW],
                              gimbal.out[GIMBAL_ACTR_PIT]);
      osKernelUnlock();

      osDelayUntil(tick);
    }
  }
}
