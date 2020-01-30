/*
	姿态解算任务。
	
	控制IMU加热到指定温度防止温漂，收集IMU数据给AHRS算法。
	解算后的数据发送给需要用的任务。
*/

/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include Board相关的头文件 */
#include "bsp_pwm.h"

/* Include Device相关的头文件。*/
#include "imu.h"

/* Include Component相关的头文件。*/
#include "ahrs.h"
#include "pid.h"
/* Include Module相关的头文件。*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
IMU_t onboard_imu;
AHRS_t gimbal_ahrs;
PID_t imu_temp_ctrl_pid;
int result = 0;

/* Private function prototypes -----------------------------------------------*/




void Task_PosEsti(const void *argument) {
	const uint32_t delay_ms = 1000U / TASK_POSESTI_FREQ_HZ;
	const Task_List_t task_list = *(Task_List_t*)argument;
	
	
	osDelay(TASK_POSESTI_INIT_DELAY);
	
	onboard_imu.received_alert = task_list.pos_esti;
	result += IMU_Init(&onboard_imu);
	
	result += IMU_StartReceiving(&onboard_imu);
	osSignalWait(IMU_SIGNAL_DATA_REDY, osWaitForever);
	result += IMU_Parse(&onboard_imu);
	
	result += AHRS_Init(&gimbal_ahrs, &onboard_imu, TASK_POSESTI_FREQ_HZ);
	
	result += PID_Init(&imu_temp_ctrl_pid, PID_MODE_DERIVATIV_NONE, 1.f/TASK_POSESTI_FREQ_HZ);
	result += PID_SetParameters(&imu_temp_ctrl_pid, .05f, .01f, 0.f, 1.f, 1.f);
	
	result += BSP_PWM_Set(BSP_PWM_IMU_HEAT, 0.f);
	result += BSP_PWM_Start(BSP_PWM_IMU_HEAT);
	
	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		result += IMU_StartReceiving(&onboard_imu);
		osSignalWait(IMU_SIGNAL_DATA_REDY, osWaitForever);
		result += IMU_Parse(&onboard_imu);
		
		result += AHRS_Update(&gimbal_ahrs, &onboard_imu);

		result += BSP_PWM_Set(BSP_PWM_IMU_HEAT, PID_Calculate(&imu_temp_ctrl_pid, 50.f, onboard_imu.data.temp, 0.f, 0.f));
		
		osDelayUntil(&previous_wake_time, delay_ms);
	}
}
