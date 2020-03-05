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
	
	
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_CTRL_CHASSIS);
	
	cd.motor_alert[0] = osThreadGetId();
	cd.motor_alert[1] = task_param->thread.ctrl_gimbal;
	cd.motor_alert[2] = task_param->thread.ctrl_shoot;
	cd.uwb_alert = task_param->thread.referee;
	cd.supercap_alert = osThreadGetId();;
	
	CAN_DeviceInit(&cd);
	dr16 = DR16_GetDevice();
	
	Chassis_Init(&chassis, CHASSIS_TYPE_MECANUM);
	chassis.dt_sec = (float)delay_tick / 1000.f;
	
	uint32_t tick = osKernelGetTickCount();
	while(1) {
		/* Task body */
		tick += delay_tick;
		
		osThreadFlagsWait(DR16_SIGNAL_DATA_REDY, osFlagsWaitAll, 0);
		Chassis_ParseCommand(&chas_ctrl, dr16);
		
		if (osThreadFlagsWait(CAN_DEVICE_SIGNAL_MOTOR_RECV, osFlagsWaitAll, delay_tick)!= osFlagsErrorTimeout) {
		
			taskENTER_CRITICAL();
			Chassis_UpdateFeedback(&chassis, &cd);
			taskEXIT_CRITICAL();
			
			Chassis_SetMode(&chassis, chas_ctrl.mode);
			Chassis_Control(&chassis, &chas_ctrl.ctrl_v);
			
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
