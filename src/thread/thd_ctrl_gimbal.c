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
#include "mod_gimbal.h"
#include "thd.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
static CAN_t can;

#ifdef MCU_DEBUG_BUILD
CMD_GimbalCmd_t gimbal_cmd;
Gimbal_t gimbal;
CAN_GimbalOutput_t gimbal_out;
UI_GimbalUI_t gimbal_ui;
#else
static CMD_GimbalCmd_t gimbal_cmd;
static Gimbal_t gimbal;
static CAN_GimbalOutput_t gimbal_out;
static UI_GimbalUI_t gimbal_ui;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * @brief 控制云台
 *
 * @param argument 未使用
 */
void Thread_CtrlGimbal(void *argument) {
  UNUSED(argument); /* 未使用argument，消除警告 */

  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_CTRL_GIMBAL;
  /* 初始化云台 */
  Gimbal_Init(&gimbal, &(runtime.cfg.robot_param->gimbal),
              runtime.cfg.gimbal_limit, (float)TASK_FREQ_CTRL_GIMBAL);

  /* 延时一段时间再开启任务 */
  xQueueReceive(runtime.msgq.can.feedback.gimbal, &can, portMAX_DELAY);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    xQueueReceive(runtime.msgq.can.feedback.gimbal, &can, 0);

    /* 读取控制指令、姿态、IMU数据 */
    xQueueReceive(runtime.msgq.gimbal.eulr_imu, &(gimbal.feedback.eulr.imu), 0);
    xQueueReceive(runtime.msgq.gimbal.gyro, &(gimbal.feedback.gyro), 0);
    xQueueReceive(runtime.msgq.cmd.gimbal, &gimbal_cmd, 0);

    vTaskSuspendAll(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
    Gimbal_UpdateFeedback(&gimbal, &can);
    Gimbal_Control(&gimbal, &gimbal_cmd, xTaskGetTickCount());
    Gimbal_PackOutput(&gimbal, &gimbal_out);
    Gimbal_PackUi(&gimbal, &gimbal_ui);
    xTaskResumeAll();

    xQueueOverwrite(runtime.msgq.can.output.gimbal, &gimbal_out);
    xQueueOverwrite(runtime.msgq.ui.gimbal, &gimbal_ui);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, delay_tick);
  }
}
