/*
	控制云台。

*/

/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include 标准库 */
/* Include Board相关的头文件 */
#include "bsp_usb.h"

/* Include Device相关的头文件 */
/* Include Component相关的头文件 */
/* Include Module相关的头文件 */
#include "gimbal.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static const uint32_t delay_ms = osKernelSysTickFrequency / TASK_FREQ_HZ_CTRL_GIMBAL;

static CAN_Device_t *cd;
static DR16_t *dr16;

static Gimbal_t gimbal;
static Gimbal_Ctrl_t gimbal_ctrl;

/* Runtime status. */
int stat_c_g = 0;
osStatus os_stat_c_g = osOK;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_CtrlGimbal(void const *argument) {
	Task_Param_t *task_param = (Task_Param_t*)argument;
	
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_CTRL_GIMBAL);
	
	cd = CAN_GetDevice();
	dr16 = DR16_GetDevice();
	
	Gimbal_Init(&gimbal);
	gimbal.dt_sec = (float)delay_ms / 1000.f;
	gimbal.imu = BMI088_GetDevice();
	
	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		/* Task body */
		
		osSignalWait(DR16_SIGNAL_DATA_REDY, 0);
		Gimbal_ParseCommand(&gimbal, &gimbal_ctrl, dr16);
		
		osSignalWait(CAN_DEVICE_SIGNAL_MOTOR_RECV, osWaitForever);
		
		taskENTER_CRITICAL();
		Gimbal_UpdateFeedback(&gimbal, cd);
		taskEXIT_CRITICAL();
		
		osEvent evt = osMessageGet(task_param->message.gimb_eulr, osWaitForever);
		if (evt.status == osEventMessage) {
			if (gimbal.imu_eulr) {
				vPortFree(gimbal.imu_eulr);
			}
			gimbal.imu_eulr = evt.value.p;
		}
		
		Gimbal_SetMode(&gimbal, gimbal_ctrl.mode);
		Gimbal_Control(&gimbal, &gimbal_ctrl.ctrl_eulr);
		
		// Check can error
		CAN_Motor_ControlGimbal(gimbal.yaw_cur_out, gimbal.pit_cur_out);
		
		osDelayUntil(&previous_wake_time, delay_ms);
	}
}
