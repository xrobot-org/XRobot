/*
	底盘任务，用于控制底盘。
	
*/


/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include 标准库 */
/* Include Board相关的头文件 */
/* Include Device相关的头文件 */
#include "can_device.h"
#include "dr16.h"

/* Include Component相关的头文件 */
#include "mixer.h"
#include "pid.h"

/* Include Module相关的头文件 */
#include "chassis.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static const uint32_t delay_ms = 1000u / TASK_CTRL_CHASSIS_FREQ_HZ;

static CAN_Device_t cd;
static Chassis_t chassis;
static Chassis_Ctrl_t *chas_ctrl = NULL;

/* Runtime status. */
int stat_c_c = 0;
osStatus os_stat_c_c = osOK;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_CtrlChassis(void const *argument) {
	const Task_Param_t *task_param = (Task_Param_t*)argument;
	
	
	/* Task Setup */
	osDelay(TASK_CTRL_CHASSIS_INIT_DELAY);
	
	/* Init hardware */
	cd.motor_alert[0] = osThreadGetId();
	cd.motor_alert[1] = task_param->thread.ctrl_gimbal;
	cd.motor_alert[2] = task_param->thread.ctrl_shoot;
	cd.uwb_alert = task_param->thread.referee;
	cd.supercap_alert = task_param->thread.ctrl_chassis;
	
	CAN_DeviceInit(&cd);
	
	Chassis_Init(&chassis, CHASSIS_TYPE_MECANUM);
	
	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		/* Task body */
		
		/* Wait for motor feedback. */
		osSignalWait(CAN_DEVICE_SIGNAL_MOTOR_RECV, osWaitForever);
		
		taskENTER_CRITICAL();
		Chassis_UpdateFeedback(&chassis, &cd);
		taskEXIT_CRITICAL();
		
		/* Try to get new rc command. */
		osSignalWait(DR16_SIGNAL_DATA_REDY, 0);
		
		osEvent evt = osMessageGet(task_param->message.chassis_ctrl_v, 0);
		if (evt.status == osEventMessage) {
			if (chas_ctrl) {
				vPortFree(chas_ctrl);
			}
			chas_ctrl = evt.value.p;
		}
		Chassis_SetMode(&chassis, chas_ctrl->mode);
		Chassis_Control(&chassis, &chas_ctrl->ctrl_v);
		
		// Check can error
		CAN_Motor_ControlChassis(
			chassis.motor_cur_out[0],
			chassis.motor_cur_out[1],
			chassis.motor_cur_out[2],
			chassis.motor_cur_out[3]);
		
		osDelayUntil(&previous_wake_time, delay_ms);
	}
}
