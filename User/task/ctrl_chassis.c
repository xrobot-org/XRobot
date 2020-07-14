/*
	底盘任务，用于控制底盘。
	
*/


/* Includes ------------------------------------------------------------------*/
#include "task\user_task.h"

#include "component\config.h"

#include "module\chassis.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static CAN_Device_t cd;

static CMD_t *cmd;

static Chassis_t chassis;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_CtrlChassis(void *argument) {
	const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_MONITOR;
	const Task_Param_t *task_param = (Task_Param_t*)argument;
	
	/* Device Setup */
	osDelay(TASK_INIT_DELAY_CTRL_CHASSIS);
	
	osThreadId_t recv_motor_allert[3] = {
		osThreadGetId(),
		task_param->thread.ctrl_gimbal,
		task_param->thread.ctrl_shoot
	};
	
	CAN_DeviceInit(&cd, recv_motor_allert, 3, task_param->thread.referee, osThreadGetId());

	/* Module Setup */
	Chassis_Init(
		&chassis,
		&(Config_GetRobot(CONFIG_ROBOT_MODEL_INFANTRY)->param.chassis),
		(float32_t)delay_tick / (float32_t)osKernelGetTickFreq());
	
	/* Task Setup */
	uint32_t tick = osKernelGetTickCount();
	while(1) {
		/* Task body */
		tick += delay_tick;
		
		uint32_t flag = CAN_DEVICE_SIGNAL_MOTOR_RECV;
		if (osThreadFlagsWait(flag, osFlagsWaitAll, delay_tick) != osFlagsErrorTimeout) {
			osMessageQueueGet(task_param->messageq.cmd, cmd, NULL, 0);
		
			osKernelLock();
			Chassis_UpdateFeedback(&chassis, &cd);
			osKernelUnlock();
			
			Chassis_Control(&chassis, &(cmd->chassis));
			
			// Check can error
			CAN_Motor_ControlChassis(
				chassis.motor_cur_out[0],
				chassis.motor_cur_out[1],
				chassis.motor_cur_out[2],
				chassis.motor_cur_out[3]);
			
			osDelayUntil(tick);
		} else {
			CAN_Motor_ControlChassis(0.f, 0.f, 0.f, 0.f);
		}
	}
}
