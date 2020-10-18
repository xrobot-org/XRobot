/*
  云台控制任务

  控制云台行为。

  从CAN总线接收底盘电机反馈，从IMU接收欧拉角和角速度，
  根据接收到的控制命令，控制电机输出。
*/

/* Includes ----------------------------------------------------------------- */
#include "module\config.h"
#include "module\gimbal.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
static CAN_t *can;

#ifdef DEBUG
CMD_GimbalCtrl_t gimbal_ctrl;
Gimbal_Feedback gimbal_feedback;
Gimbal_t gimbal;
#else
static CMD_GimbalCtrl_t gimbal_ctrl;
static Gimbal_Feedback gimbal_feedback;
static Gimbal_t gimbal;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */
void Task_CtrlGimbal(void *argument) {
  (void)argument;
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_CTRL_GIMBAL;

  /* Task Setup */
  osDelay(TASK_INIT_DELAY_CTRL_GIMBAL);

  while ((can = CAN_GetDevice()) == NULL) {
    osDelay(delay_tick);
  }

  Gimbal_Init(&gimbal, &(task_runtime.robot_param->gimbal),
              (float)TASK_FREQ_HZ_CTRL_GIMBAL);

  uint32_t tick = osKernelGetTickCount();
  uint32_t wakeup = HAL_GetTick();
  while (1) {
#ifdef DEBUG
    task_runtime.stack_water_mark.ctrl_gimbal = osThreadGetStackSpace(NULL);
#endif
    /* Task body */
    tick += delay_tick;

    const uint32_t flag = SIGNAL_CAN_MOTOR_RECV;
    if (osThreadFlagsWait(flag, osFlagsWaitAll, delay_tick) != flag) {
      CAN_Motor_ControlGimbal(0.0f, 0.0f);

    } else {
      osMessageQueueGet(task_runtime.msgq.gimbal.eulr_imu,
                        &(gimbal_feedback.eulr.imu), NULL, 0);
      osMessageQueueGet(task_runtime.msgq.gimbal.gyro, &(gimbal_feedback.gyro),
                        NULL, 0);
      osMessageQueueGet(task_runtime.msgq.cmd.gimbal, &gimbal_ctrl, NULL, 0);

      osKernelLock();
      const uint32_t now = HAL_GetTick();
      Gimbal_CANtoFeedback(&gimbal_feedback, can);
      Gimbal_Control(&gimbal, &gimbal_feedback, &gimbal_ctrl,
                     (float)(now - wakeup) / 1000.0f);
      wakeup = now;
      CAN_Motor_ControlGimbal(gimbal.out[GIMBAL_ACTR_YAW_IDX],
                              gimbal.out[GIMBAL_ACTR_PIT_IDX]);
      osKernelUnlock();

      osDelayUntil(tick);
    }
  }
}
