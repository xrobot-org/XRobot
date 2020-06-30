/*
	底盘任务，用于控制底盘。
	
*/


/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include 标准库 */
/* Include Board相关的头文件 */
#include "bsp_usb.h"

/* Include Device相关的头文件 */
/* Include Component相关的头文件 */
#include "robot_config.h"

/* Include Module相关的头文件 */
#include "chassis.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static CAN_Device_t cd;
static DR16_t *dr16;

static Chassis_t chassis;
static Chassis_Ctrl_t chas_ctrl;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_CtrlChassis(void *argument) {
	const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_MONITOR;
	const Task_Param_t *task_param = (Task_Param_t*)argument;
	
	/* Device Setup */
	osDelay(TASK_INIT_DELAY_CTRL_CHASSIS);
	
	osThreadId recv_motor_allert[3] = {
		osThreadGetId(),
		task_param->thread.ctrl_gimbal,
		task_param->thread.ctrl_shoot
	};
	
	CAN_DeviceInit(&cd, recv_motor_allert, 3, task_param->thread.referee, osThreadGetId());
	dr16 = DR16_GetDevice();
	
	/* Module Setup */
	Chassis_Init(&chassis, &(RobotConfig_Get(ROBOT_CONFIG_MODEL_INFANTRY)->param.chassis));
	chassis.dt_sec = (float32_t)delay_tick / (float32_t)osKernelGetTickFreq();
	
	/* Task Setup */
	uint32_t tick = osKernelGetTickCount();
	while(1) {
		/* Task body */
		tick += delay_tick;
		
		uint32_t flag = DR16_SIGNAL_DATA_REDY | CAN_DEVICE_SIGNAL_MOTOR_RECV;
		if (osThreadFlagsWait(flag, osFlagsWaitAll, delay_tick) != osFlagsErrorTimeout) {
			Chassis_ParseCommand(&chas_ctrl, dr16);
		
			osKernelLock();
			Chassis_UpdateFeedback(&chassis, &cd);
			osKernelUnlock();
			
			Chassis_Control(&chassis, &chas_ctrl);
			
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
