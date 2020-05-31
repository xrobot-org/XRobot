/*
	控制云台。

*/

/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include 标准库 */
/* Include Board相关的头文件 */
#include "bsp_usb.h"
#include "bsp_mm.h"

/* Include Device相关的头文件 */
/* Include Component相关的头文件 */
#include "robot_config.h"

/* Include Module相关的头文件 */
#include "gimbal.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static CAN_Device_t *cd;
static DR16_t *dr16;

static Gimbal_t gimbal;
static Gimbal_Ctrl_t gimbal_ctrl;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_CtrlGimbal(void *argument) {
	const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_CTRL_GIMBAL;
	const Task_Param_t *task_param = (Task_Param_t*)argument;
	
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_CTRL_GIMBAL);
	
	cd = CAN_GetDevice();
	dr16 = DR16_GetDevice();
	
	Gimbal_Init(&gimbal, &(RobotConfig_Get(ROBOT_CONFIG_MODEL_INFANTRY)->gimbal_param));
	gimbal.dt_sec = (float)delay_tick / (float)osKernelGetTickFreq();
	gimbal.imu = BMI088_GetDevice();
	
	uint32_t tick = osKernelGetTickCount();
	while(1) {
		/* Task body */
		tick += delay_tick;
		
		osThreadFlagsWait(DR16_SIGNAL_DATA_REDY, osFlagsWaitAll, 0);
		Gimbal_ParseCommand(&gimbal, &gimbal_ctrl, dr16);
		
		if (osThreadFlagsWait(CAN_DEVICE_SIGNAL_MOTOR_RECV, osFlagsWaitAll, delay_tick) != osFlagsErrorTimeout) {
			osStatus os_status = osMessageQueueGet(task_param->messageq.gimb_eulr, gimbal.imu_eulr, NULL, 0);
			
			osKernelLock();
			Gimbal_UpdateFeedback(&gimbal, cd);
			osKernelUnlock();
			
			Gimbal_SetMode(&gimbal, gimbal_ctrl.mode);
			Gimbal_Control(&gimbal, &gimbal_ctrl.ctrl_eulr);
			
			// Check can error
			CAN_Motor_ControlGimbal(gimbal.yaw_cur_out, gimbal.pit_cur_out);
			
			osDelayUntil(tick);
		} else {
			CAN_Motor_ControlGimbal(0.f, 0.f);
		}
	}
}
