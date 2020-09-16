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

#ifdef DEBUG
CMD_Gimbal_Ctrl_t gimbal_ctrl;
Gimbal_t gimbal;
#else
static CMD_Gimbal_Ctrl_t gimbal_ctrl;
static Gimbal_t gimbal;
#endif

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
              (float)delay_tick / (float)osKernelGetTickFreq());

  uint32_t tick = osKernelGetTickCount();
  while (1) {
#ifdef DEBUG
    task_param->stack_water_mark.ctrl_gimbal = osThreadGetStackSpace(NULL);
#endif
    /* Task body */
    tick += delay_tick;

    const uint32_t flag = SIGNAL_CAN_MOTOR_RECV;
    if (osThreadFlagsWait(flag, osFlagsWaitAll, delay_tick) != flag) {
      CAN_Motor_ControlGimbal(0.f, 0.f);

    } else {
      osMessageQueueGet(task_param->msgq.gimbal_eulr_imu, &(gimbal.feedback.eulr.imu), NULL, 0);
      osMessageQueueGet(task_param->msgq.gimbal_gyro, &(gimbal.feedback.gyro), NULL, 0);
      osMessageQueueGet(task_param->msgq.cmd.gimbal, &gimbal_ctrl, NULL, 0);

      osKernelLock();
      Gimbal_UpdateFeedback(&gimbal, can);
      Gimbal_Control(&gimbal, &gimbal_ctrl);
      CAN_Motor_ControlGimbal(gimbal.out[GIMBAL_ACTR_YAW],
                              gimbal.out[GIMBAL_ACTR_PIT]);
      osKernelUnlock();

      osDelayUntil(tick);
    }
  }
}
