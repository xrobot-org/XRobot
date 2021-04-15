/**
 * @file ctrl_gimbal.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 云台控制任务
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 * 通过消息队列收集云台控制需要的电机反馈、欧拉角、角速度，
 * 运行gimbal模组
 * 通过消息队列发送云台控制输出的数据
 *
 */

/* Includes ----------------------------------------------------------------- */
#include "module/gimbal.h"
#include "task/user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
static CAN_t can;

#ifdef DEBUG
CMD_GimbalCmd_t gimbal_cmd;
Gimbal_t gimbal;
CAN_GimbalOutput_t gimbal_out;
Referee_GimbalUI_t gimbal_ui;
#else
static CMD_GimbalCmd_t gimbal_cmd;
static Gimbal_t gimbal;
static CAN_GimbalOutput_t gimbal_out;
static Referee_GimbalUI_t gimbal_ui;
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

  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_CTRL_GIMBAL;
  /* 初始化云台 */
  Gimbal_Init(&gimbal, &(task_runtime.cfg.robot_param->gimbal),
              task_runtime.cfg.gimbal_limit, (float)TASK_FREQ_CTRL_GIMBAL);

  /* 延时一段时间再开启任务 */
  osMessageQueueGet(task_runtime.msgq.can.feedback.gimbal, &can, NULL,
                    osWaitForever);
  uint32_t tick = osKernelGetTickCount(); /* 控制任务运行频率的计时 */
  while (1) {
#ifdef DEBUG
    /* 记录任务所使用的的栈空间 */
    task_runtime.stack_water_mark.ctrl_gimbal =
        osThreadGetStackSpace(osThreadGetId());
#endif

    osMessageQueueGet(task_runtime.msgq.can.feedback.gimbal, &can, NULL, 0);
    /* 读取控制指令、姿态、IMU数据 */
    osMessageQueueGet(task_runtime.msgq.gimbal.eulr_imu,
                      &(gimbal.feedback.eulr.imu), NULL, 0);
    osMessageQueueGet(task_runtime.msgq.gimbal.gyro, &(gimbal.feedback.gyro),
                      NULL, 0);
    osMessageQueueGet(task_runtime.msgq.cmd.gimbal, &gimbal_cmd, NULL, 0);

    osKernelLock(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
    Gimbal_UpdateFeedback(&gimbal, &can);
    Gimbal_Control(&gimbal, &gimbal_cmd, tick);
    Gimbal_DumpOutput(&gimbal, &gimbal_out);

    osKernelUnlock();
    osMessageQueueReset(task_runtime.msgq.can.output.gimbal);
    osMessageQueuePut(task_runtime.msgq.can.output.gimbal, &gimbal_out, 0, 0);

    Gimbal_DumpUI(&gimbal, &gimbal_ui);
    osMessageQueueReset(task_runtime.msgq.ui.gimbal);
    osMessageQueuePut(task_runtime.msgq.ui.gimbal, &gimbal_ui, 0, 0);

    tick += delay_tick; /* 计算下一个唤醒时刻 */
    osDelayUntil(tick); /* 运行结束，等待下一次唤醒 */
  }
}
