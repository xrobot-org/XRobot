/*
	控制射击。

*/

/* Includes ------------------------------------------------------------------*/
#include "user_task.h"

#include "component\config.h"

#include "module\shoot.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static CAN_Device_t *cd;
static CMD_t *cmd;
static Shoot_t shoot;

/* Private function ----------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_CtrlShoot(void *argument) {
	const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_CTRL_SHOOT;
	Task_Param_t *task_param = (Task_Param_t*)argument;
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_CTRL_SHOOT);
	
	cd = CAN_GetDevice();

	Shoot_Init(
		&shoot, 
		&(Config_GetRobot(CONFIG_ROBOT_MODEL_INFANTRY)->param.shoot),
		(float32_t)delay_tick / (float32_t)osKernelGetTickFreq());
	
	uint32_t tick = osKernelGetTickCount();
	while(1) {
		/* Task body */
		tick += delay_tick;
		
		uint32_t flag = CAN_DEVICE_SIGNAL_MOTOR_RECV;
		if (osThreadFlagsWait(flag, osFlagsWaitAll, delay_tick) != osFlagsErrorTimeout) {
			osMessageQueueGet(task_param->messageq.cmd, cmd, NULL, 0);
			
			osKernelLock();
			Shoot_UpdateFeedback(&shoot, cd);
			osKernelUnlock();
			
			Shoot_Control(&shoot, &(cmd->shoot));
			
			// TODO: Check can error.
			CAN_Motor_ControlShoot(
				shoot.fric_cur_out[0],
				shoot.fric_cur_out[1],
				shoot.trig_cur_out
			);
			
			osDelayUntil(tick);
		} else {
			CAN_Motor_ControlShoot(0.f, 0.f, 0.f);
		}
#ifdef DEBUG
		task_param->stack_water_mark.cli = uxTaskGetStackHighWaterMark(NULL);
#endif
	}
}
