/*
	控制云台。

*/

/* Includes ------------------------------------------------------------------*/
#include "user_task.h"

#include "component\config.h"

#include "module\gimbal.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static CAN_Device_t *cd;

static CMD_t *cmd;

static Gimbal_t gimbal;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_CtrlGimbal(void *argument) {
	const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_CTRL_GIMBAL;
	const Task_Param_t *task_param = (Task_Param_t*)argument;
	
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_CTRL_GIMBAL);
	
	cd = CAN_GetDevice();
	
	Gimbal_Init(
		&gimbal, 
		&(Config_GetRobot(CONFIG_ROBOT_MODEL_INFANTRY)->param.gimbal),
		(float32_t)delay_tick / (float32_t)osKernelGetTickFreq(),
		BMI088_GetDevice());
	
	uint32_t tick = osKernelGetTickCount();
	while(1) {
		/* Task body */
		tick += delay_tick;
		
		uint32_t flag = CAN_DEVICE_SIGNAL_MOTOR_RECV;
		if (osThreadFlagsWait(flag, osFlagsWaitAll, delay_tick) != osFlagsErrorTimeout) {
			
			osMessageQueueGet(task_param->messageq.gimb_eulr, gimbal.imu_eulr, NULL, 0);
			osMessageQueueGet(task_param->messageq.cmd, cmd, NULL, 0);
			
			osKernelLock();
			Gimbal_UpdateFeedback(&gimbal, cd);
			osKernelUnlock();
			
			Gimbal_Control(&gimbal, &(cmd->gimbal));
			
			CAN_Motor_ControlGimbal(gimbal.yaw_cur_out, gimbal.pit_cur_out);
			
			osDelayUntil(tick);
		} else {
			CAN_Motor_ControlGimbal(0.f, 0.f);
		}
	}
}
