/*
	控制射击。

*/

/* Includes ------------------------------------------------------------------*/
#include "user_task.h"

#include "module\robot.h"
#include "module\shoot.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static CAN_t *can;
static CMD_t *cmd;
static Shoot_t shoot;

/* Private function ----------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_CtrlShoot(void *argument) {
	const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_CTRL_SHOOT;
	Task_Param_t *task_param = (Task_Param_t*)argument;
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_CTRL_SHOOT);
	
	while((can = CAN_GetDevice()) == NULL) {
		osDelay(delay_tick);
	}

	Shoot_Init(
		&shoot, 
		&(task_param->config_robot->param.shoot),
		(float)delay_tick / (float)osKernelGetTickFreq());
	
	uint32_t tick = osKernelGetTickCount();
	while(1) {
#ifdef DEBUG
		task_param->stack_water_mark.ctrl_shoot = osThreadGetStackSpace(NULL);
#endif
		/* Task body */
		tick += delay_tick;
		
		uint32_t flag = SIGNAL_CAN_MOTOR_RECV;
		if (osThreadFlagsWait(flag, osFlagsWaitAll, delay_tick) == osFlagsErrorTimeout) {
			CAN_Motor_ControlShoot(0.f, 0.f, 0.f);
			
		} else {
			osMessageQueueGet(task_param->messageq.cmd, cmd, NULL, 0);
			
			osKernelLock();
			Shoot_UpdateFeedback(&shoot, can);
			Shoot_Control(&shoot, &(cmd->shoot));
			CAN_Motor_ControlShoot(
				shoot.fric_cur_out[0],
				shoot.fric_cur_out[1],
				shoot.trig_cur_out
			);
			osKernelUnlock();
			
			osDelayUntil(tick);
		}
	}
}
