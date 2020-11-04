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
static CAN_t can;

#ifdef DEBUG
CMD_GimbalCmd_t gimbal_cmd;
Gimbal_Feedback_t gimbal_feedback;
Gimbal_t gimbal;
CAN_GimbalOutput_t gimbal_out;
#else
static CMD_GimbalCmd_t gimbal_cmd;
static Gimbal_Feedback_t gimbal_feedback;
static Gimbal_t gimbal;
static CAN_GimbalOutput_t gimbal_out;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/*!
 * \brief 控制云台
 *
 * \param argument 未使用
 */
void Task_CtrlGimbal(void *argument) {
  (void)argument; /* 未使用argument，消除警告 */

  /* 计算任务运行到指定频率，需要延时的时间 */
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_CTRL_GIMBAL;

  /* 初始化云台 */
  Gimbal_Init(&gimbal, &(task_runtime.robot_param->gimbal),
              (float)TASK_FREQ_CTRL_GIMBAL);

  /* 延时一段时间再开启任务 */
  osMessageQueueGet(task_runtime.msgq.motor.feedback.gimbal,
                    &(can.gimbal_motor), NULL, delay_tick);

  uint32_t tick = osKernelGetTickCount(); /* 控制任务运行频率的计时 */
  uint32_t wakeup = HAL_GetTick(); /* 计算任务运行间隔的计时 */
  while (1) {
#ifdef DEBUG
    /* 记录任务所使用的的栈空间 */
    task_runtime.stack_water_mark.ctrl_gimbal = osThreadGetStackSpace(NULL);
#endif
    tick += delay_tick; /* 计算下一个唤醒时刻 */

    /* 等待接收CAN总线新数据 */
    if (osMessageQueueGet(task_runtime.msgq.motor.feedback.gimbal,
                          &can, NULL, delay_tick) != osOK) {
      /* 如果没有接收到新数据，则将输出置零，不进行控制 */
      CAN_ResetGimbalOut(&gimbal_out);
      osMessageQueuePut(task_runtime.msgq.motor.output.gimbal, &gimbal_out, 0,
                        0);

    } else {
      /* 继续读取控制指令、姿态、IMU数据 */
      osMessageQueueGet(task_runtime.msgq.gimbal.eulr_imu,
                        &(gimbal_feedback.eulr.imu), NULL, 0);
      osMessageQueueGet(task_runtime.msgq.gimbal.gyro, &(gimbal_feedback.gyro),
                        NULL, 0);
      osMessageQueueGet(task_runtime.msgq.cmd.gimbal, &gimbal_cmd, NULL, 0);

      osKernelLock(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
      const uint32_t now = HAL_GetTick();
      Gimbal_CANtoFeedback(&gimbal_feedback, &can);
      Gimbal_Control(&gimbal, &gimbal_feedback, &gimbal_cmd,
                     (float)(now - wakeup) / 1000.0f);
      Gimbal_DumpOutput(&gimbal, &gimbal_out);
      osMessageQueuePut(task_runtime.msgq.motor.output.gimbal, &gimbal_out, 0,
                        0);
      wakeup = now;
      osKernelUnlock();

      osDelayUntil(tick); /* 运行结束，等待下一次唤醒 */
    }
  }
}
