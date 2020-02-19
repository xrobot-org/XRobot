/*
	控制云台。

*/

/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include 标准库 */
/* Include Board相关的头文件 */
/* Include Device相关的头文件 */
#include "can_device.h"

/* Include Component相关的头文件 */
#include "ahrs.h"

/* Include Module相关的头文件 */
#include "gimbal.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static const uint32_t delay_ms = 1000u / TASK_CTRL_GIMBAL_FREQ_HZ;

static CAN_Device_t *cd;
static Gimbal_t gimbal;

/* Runtime status. */
int stat_c_g = 0;
osStatus os_stat_c_g = osOK;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_CtrlGimbal(void const *argument) {
	Task_Param_t *task_param = (Task_Param_t*)argument;
	
	/* Task Setup */
	osSignalWait(TASK_SIGNAL_CTRL_CHASSIS_READY, TASK_CTRL_CHASSIS_INIT_DELAY);
	osSignalWait(TASK_SIGNAL_POSESTI_READY, TASK_CTRL_CHASSIS_INIT_DELAY);
	
	cd = CAN_GetDevice();
	
	Gimbal_Init(&gimbal);
	
	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		/* Task body */
		
		/* Try to get new command. */
		osEvent evt = osMessageGet(task_param->message.chassis_ctrl_v, 1u);
		if (evt.status == osEventMessage) {
			if (gimbal.ctrl_eulr) {
				vPortFree(gimbal.ctrl_eulr);
			}
			gimbal.ctrl_eulr = evt.value.p;
		}
		
		/* Wait for new eulr data. */
		evt = osMessageGet(task_param->message.gimb_eulr, osWaitForever);
		if (evt.status == osEventMessage) {
			if (gimbal.gimb_eulr) {
				vPortFree(gimbal.gimb_eulr);
			}
			gimbal.gimb_eulr = evt.value.p;
		}
		
		/* Wait for new IMU data. */
		evt = osMessageGet(task_param->message.imu, osWaitForever);
		if (evt.status == osEventMessage) {
			if (gimbal.imu) {
				vPortFree(gimbal.imu);
			}
			gimbal.imu = evt.value.p;
		}
		
		/* Wait for motor feedback. */
		osSignalWait(CAN_DEVICE_SIGNAL_GIMBAL_RECV, osWaitForever);
		
		taskENTER_CRITICAL();
		Gimbal_UpdateFeedback(&gimbal, cd);
		taskEXIT_CRITICAL();
		
		Gimbal_Control(&gimbal);
		
		// Check can error
		CAN_Motor_ControlGimbal(gimbal.yaw_cur_out, gimbal.pit_cur_out);
		
		osDelayUntil(&previous_wake_time, delay_ms);
	}
}
