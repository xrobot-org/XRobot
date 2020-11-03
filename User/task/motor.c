/*
  底盘控制任务

  控制底盘行为。

  从CAN总线接收底盘电机反馈，根据接收到的控制命令，控制电机输出。
*/

/* Includes ----------------------------------------------------------------- */
#include "device\can.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */

#ifdef DEBUG
CAN_t can;
CAN_Output_t can_out;
CAN_MotorRawRx_t can_motor_rx;
#else
static CAN_t can;
static CAN_Output_t can_out;
static CAN_MotorRawRx_t can_motor_rx;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */
void Task_Motor(void *argument) {
  (void)argument;
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_MOTOR;

  task_runtime.msgq.motor.feedback.chassis =
      osMessageQueueNew(6u, sizeof(CAN_ChassisMotor_t), NULL);

  task_runtime.msgq.motor.feedback.gimbal =
      osMessageQueueNew(6u, sizeof(CAN_GimbalMotor_t), NULL);

  task_runtime.msgq.motor.feedback.shoot =
      osMessageQueueNew(6u, sizeof(CAN_ShootMotor_t), NULL);

  task_runtime.msgq.motor.output.chassis =
      osMessageQueueNew(6u, sizeof(CAN_ChassisOutput_t), NULL);

  task_runtime.msgq.motor.output.gimbal =
      osMessageQueueNew(6u, sizeof(CAN_GimbalOutput_t), NULL);

  task_runtime.msgq.motor.output.shoot =
      osMessageQueueNew(6u, sizeof(CAN_ShootOutput_t), NULL);

  /* Device Setup */
  CAN_Init(&can);

  /* Task Setup */
  while (1) {
#ifdef DEBUG
    task_runtime.stack_water_mark.motor = osThreadGetStackSpace(NULL);
#endif
    // 接收消息
    while (osMessageQueueGet(can.msgq_can2motor, &can_motor_rx, 0,
                             delay_tick) == osOK) {
      osKernelLock();
      CAN_Motor_StoreMsg(&can, &can_motor_rx);

      //电机凑够，向指定任务发送
      if (CAN_Motor_CheckFlag(&can, MOTOR_REC_CHASSIS_FINISHED)) {
        osMessageQueueReset(task_runtime.msgq.motor.feedback.chassis);
        osMessageQueuePut(task_runtime.msgq.motor.feedback.chassis,
                          &(can.chassis_motor), 0, 0);
        CAN_Motor_ClearFlag(&can, MOTOR_REC_CHASSIS_FINISHED);
      }

      if (CAN_Motor_CheckFlag(&can, MOTOR_REC_GIMBAL_FINISHED)) {
        osMessageQueueReset(task_runtime.msgq.motor.feedback.gimbal);
        osMessageQueuePut(task_runtime.msgq.motor.feedback.gimbal,
                          &(can.gimbal_motor), 0, 0);
        CAN_Motor_ClearFlag(&can, MOTOR_REC_GIMBAL_FINISHED);
      }

      if (CAN_Motor_CheckFlag(&can, MOTOR_REC_SHOOT_FINISHED)) {
        osMessageQueueReset(task_runtime.msgq.motor.feedback.shoot);
        osMessageQueuePut(task_runtime.msgq.motor.feedback.shoot,
                          &(can.shoot_motor), 0, 0);
        CAN_Motor_ClearFlag(&can, MOTOR_REC_SHOOT_FINISHED);
      }

      osKernelUnlock();

      if (osMessageQueueGet(task_runtime.msgq.motor.output.chassis,
                            &(can_out.chassis), 0, 0) == osOK) {
        CAN_Motor_Control(CAN_MOTOR_GROUT_CHASSIS, &can_out);
      }

      if (osMessageQueueGet(task_runtime.msgq.motor.output.gimbal,
                            &(can_out.gimbal), 0, 0) == osOK) {
        CAN_Motor_Control(CAN_MOTOR_GROUT_GIMBAL1, &can_out);
      }

      if (osMessageQueueGet(task_runtime.msgq.motor.output.shoot,
                            &(can_out.shoot), 0, 0) == osOK) {
        CAN_Motor_Control(CAN_MOTOR_GROUT_SHOOT1, &can_out);
      }
    }
  }
}
