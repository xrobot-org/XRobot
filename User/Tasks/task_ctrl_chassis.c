/*
	底盘任务，用于控制底盘。
	
*/


/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include 标准库 */
/* Include Board相关的头文件 */
/* Include Device相关的头文件 */
#include "can_device.h"

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
static int result = 0;
static osStatus os_status = osOK;

static CAN_Device_t cd;
static Chassis_t chassis;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_CtrlChassis(void const *argument) {
	const Task_Param_t *task_param = (Task_Param_t*)argument;
	
	
	/* Task Setup */
	osDelay(TASK_CTRL_CHASSIS_INIT_DELAY);
	
	/* Init hardware */
	cd.chassis_alert = task_param->thread.ctrl_chassis;
	cd.gimbal_alert = task_param->thread.ctrl_gimbal;
	cd.uwb_alert = task_param->thread.referee;
	cd.supercap_alert = task_param->thread.ctrl_chassis;
	
	CAN_DeviceInit(&cd);
	
	Chassis_Init(&chassis, CHASSIS_TYPE_MECANUM);
	
	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		/* Task */
		
		/* Wait for new rc info. */
		osEvent evt = osMessageGet(task_param->message.chassis_ctrl_v, 1u);
		if (evt.status == osEventMessage) {
			if (chassis.robot_ctrl_v != NULL) {
				osPoolFree(task_param->pool.chassis_ctrl_v, chassis.robot_ctrl_v);
			}
			chassis.robot_ctrl_v = evt.value.p;
		}
		
		/* Wait for motor feedback. */
		osSignalWait(CAN_DEVICE_SIGNAL_CHASSIS_RECV, osWaitForever);
		Chassis_UpdateFeedback(&chassis, &cd);
		
		Chassis_Control(&chassis);
		
		// Check can error
		CAN_Motor_ControlChassis(
			chassis.motor_cur_out[0],
			chassis.motor_cur_out[1],
			chassis.motor_cur_out[2],
			chassis.motor_cur_out[3]);
		
		osDelayUntil(&previous_wake_time, delay_ms);
	}
}
