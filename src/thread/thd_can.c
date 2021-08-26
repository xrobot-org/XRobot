/**
 * @file can.c
 * @author Qu Shen (503578404@qq.com)
 * @brief CAN总线数据处理
 * @version 1.0.0
 * @date 2021-04-14
 *
 * @copyright Copyright (c) 2021
 *
 * 从消息队列中得到CAN总线收到的原始数据
 * 解析后分组存放
 * 根据需要发给对应的任务
 *
 */

/* Includes ----------------------------------------------------------------- */
#include "dev_can.h"
#include "dev_referee.h"
#include "thd.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */

#ifdef MCU_DEBUG_BUILD
CAN_t can;
CAN_Output_t can_out;
CAN_RawRx_t can_rx;
#else
static CAN_t can;
static CAN_Output_t can_out;
static CAN_RawRx_t can_rx;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */
void Thread_CAN(void *argument) {
  UNUSED(argument);
  const uint32_t delay_tick = pdMS_TO_TICKS(1000 / TASK_FREQ_CAN);

  /* Device Setup */
  CAN_Init(&can, &runtime.cfg.robot_param->can);

  uint32_t previous_wake_time = xTaskGetTickCount();

  /* Task Setup */
  while (1) {
    while (xQueueReceive(can.msgq_raw, &can_rx, 0) == pdPASS) {
      CAN_StoreMsg(&can, &can_rx);
    }
    xQueueOverwrite(runtime.msgq.can.feedback.chassis, &can);

    xQueueOverwrite(runtime.msgq.can.feedback.gimbal, &can);

    xQueueOverwrite(runtime.msgq.can.feedback.launcher, &can);

    if (CAN_CheckFlag(&can, CAN_REC_CAP_FINISHED, true)) {
      xQueueOverwrite(runtime.msgq.can.feedback.cap, &can);
    } else {
      // Error Handle
    }

    if (CAN_CheckFlag(&can, CAN_REC_TOF_FINISHED, true)) {
      xQueueOverwrite(runtime.msgq.can.feedback.tof, &can);
    } else {
      // Error Handle
    }

    if (xQueueReceive(runtime.msgq.can.output.chassis, &(can_out.chassis), 0) ==
        pdPASS) {
      CAN_Motor_Control(CAN_MOTOR_GROUT_CHASSIS, &can_out, &can);
    }

    if (xQueueReceive(runtime.msgq.can.output.gimbal, &(can_out.gimbal), 0) ==
        pdPASS) {
      CAN_Motor_Control(CAN_MOTOR_GROUT_GIMBAL1, &can_out, &can);
    }

    if (xQueueReceive(runtime.msgq.can.output.launcher, &(can_out.launcher),
                      0) == pdPASS) {
      CAN_Motor_Control(CAN_MOTOR_GROUT_LAUNCHER1, &can_out, &can);
    }

    if (xQueueReceive(runtime.msgq.can.output.cap, &(can_out.cap), 0) ==
        pdPASS) {
      CAN_Cap_Control(&(can_out.cap), &can);
    }
    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(previous_wake_time, delay_tick);
  }
}
