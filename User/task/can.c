/*
  CAN总线数据处理

  处理CAN总线收到的电机电容数据。
*/

/* Includes ----------------------------------------------------------------- */
#include "device\can.h"

#include "device\referee.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */

#ifdef DEBUG
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
void Task_Can(void *argument) {
  (void)argument;
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_CAN;

  /* Device Setup */
  CAN_Init(&can);

  /* Task Setup */
  while (1) {
#ifdef DEBUG
    task_runtime.stack_water_mark.can = osThreadGetStackSpace(NULL);
#endif
    /* 接收消息 */
    while (osMessageQueueGet(can.msgq_raw, &can_rx, 0, delay_tick) == osOK) {
      osKernelLock();

      CAN_StoreMsg(&can, &can_rx);

      /* 电机凑够，向指定任务发送 */
      if (CAN_CheckFlag(&can, CAN_REC_CHASSIS_FINISHED)) {
        osMessageQueueReset(task_runtime.msgq.can.feedback.chassis);
        osMessageQueuePut(task_runtime.msgq.can.feedback.chassis, &can, 0, 0);
        CAN_ClearFlag(&can, CAN_REC_CHASSIS_FINISHED);
      }

      if (CAN_CheckFlag(&can, CAN_REC_GIMBAL_FINISHED)) {
        osMessageQueueReset(task_runtime.msgq.can.feedback.gimbal);
        osMessageQueuePut(task_runtime.msgq.can.feedback.gimbal, &can, 0, 0);
        CAN_ClearFlag(&can, CAN_REC_GIMBAL_FINISHED);
      }

      if (CAN_CheckFlag(&can, CAN_REC_SHOOT_FINISHED)) {
        osMessageQueueReset(task_runtime.msgq.can.feedback.shoot);
        osMessageQueuePut(task_runtime.msgq.can.feedback.shoot, &can, 0, 0);
        CAN_ClearFlag(&can, CAN_REC_SHOOT_FINISHED);
      }

      if (CAN_CheckFlag(&can, CAN_REC_CAP_FINISHED)) {
        osMessageQueueReset(task_runtime.msgq.can.feedback.cap);
        osMessageQueuePut(task_runtime.msgq.can.feedback.cap, &can, 0, 0);
        CAN_ClearFlag(&can, CAN_REC_SHOOT_FINISHED);
      }

      osKernelUnlock();

      if (osMessageQueueGet(task_runtime.msgq.can.output.chassis,
                            &(can_out.chassis), 0, 0) == osOK) {
        CAN_Motor_Control(CAN_MOTOR_GROUT_CHASSIS, &can_out);
      }

      if (osMessageQueueGet(task_runtime.msgq.can.output.gimbal,
                            &(can_out.gimbal), 0, 0) == osOK) {
        CAN_Motor_Control(CAN_MOTOR_GROUT_GIMBAL1, &can_out);
      }

      if (osMessageQueueGet(task_runtime.msgq.can.output.shoot,
                            &(can_out.shoot), 0, 0) == osOK) {
        CAN_Motor_Control(CAN_MOTOR_GROUT_SHOOT1, &can_out);
      }

      if (osMessageQueueGet(task_runtime.msgq.can.output.cap, &(can_out.cap), 0,
                            0) == osOK) {
        CAN_Cap_Control(&(can_out.cap));
      }
    }
  }
}
