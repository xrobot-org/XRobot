/*
  云台控制任务

  控制云台行为。

  从CAN总线接收底盘电机反馈，从IMU接收欧拉角和角速度，
  根据接收到的控制命令，控制电机输出。
*/

/* Includes ----------------------------------------------------------------- */
#include "module\gimbal.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
static CAN_t can;

#ifdef DEBUG
CMD_GimbalCmd_t gimbal_cmd;
Gimbal_t gimbal;
CAN_GimbalOutput_t gimbal_out;
#else
static CMD_GimbalCmd_t gimbal_cmd;
static Gimbal_t gimbal;
static CAN_GimbalOutput_t gimbal_out;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * \brief 控制云台
 *
 * \param argument 未使用
 */
void Task_CtrlGimbal(void *argument) {
  (void)argument; /* 未使用argument，消除警告 */

  /* 计算任务运行到指定频率需要等待的tick数 */
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_CTRL_GIMBAL;

  /* 初始化云台 */
  Gimbal_Init(&gimbal, &(task_runtime.cfg.robot_param->gimbal),
              task_runtime.cfg.gimbal_limit, (float)TASK_FREQ_CTRL_GIMBAL);

  /* 延时一段时间再开启任务 */
  osMessageQueueGet(task_runtime.msgq.can.feedback.gimbal, &can, NULL,
                    osWaitForever);

  uint32_t tick = osKernelGetTickCount(); /* 控制任务运行频率的计时 */
  uint32_t wakeup = HAL_GetTick(); /* 计算任务运行间隔的计时 */
  while (1) {
#ifdef DEBUG
    /* 记录任务所使用的的栈空间 */
    task_runtime.stack_water_mark.ctrl_gimbal = osThreadGetStackSpace(NULL);
#endif
    tick += delay_tick; /* 计算下一个唤醒时刻 */

    /* 读取CAN电机指令、控制指令、姿态、IMU数据 */
    osMessageQueueGet(task_runtime.msgq.can.feedback.gimbal, &can, NULL, 0);
    osMessageQueueGet(task_runtime.msgq.gimbal.eulr_imu,
                      &(gimbal.feedback.eulr.imu), NULL, 0);
    osMessageQueueGet(task_runtime.msgq.gimbal.gyro, &(gimbal.feedback.gyro),
                      NULL, 0);
    osMessageQueueGet(task_runtime.msgq.cmd.gimbal, &gimbal_cmd, NULL, 0);

    osKernelLock(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
    const uint32_t now = HAL_GetTick();
    Gimbal_UpdateFeedback(&gimbal, &can);
    Gimbal_Control(&gimbal, &gimbal_cmd, (float)(now - wakeup) / 1000.0f);
    Gimbal_DumpOutput(&gimbal, &gimbal_out);
    wakeup = now;
    osKernelUnlock();

    osMessageQueuePut(task_runtime.msgq.can.output.gimbal, &gimbal_out, 0, 0);
    osDelayUntil(tick); /* 运行结束，等待下一次唤醒 */
  }
}
