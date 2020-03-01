/*
	姿态解算任务。
	
	控制IMU加热到指定温度防止温漂，收集IMU数据给AHRS算法。
	解算后的数据发送给需要用的任务。
*/

/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include 标准库 */
#include <string.h>

/* Include Board相关的头文件 */
#include "bsp_pwm.h"
#include "bsp_usb.h"

/* Include Device相关的头文件 */
#include "bmi088.h"
#include "ist8310.h"

/* Include Component相关的头文件 */
#include "ahrs.h"
#include "pid.h"

/* Include Module相关的头文件 */
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
BMI088_t bmi088;
IST8310_t ist8310;

AHRS_t gimbal_ahrs;

/* Debug */
AHRS_Eulr_t debug_eulr;

static PID_t imu_temp_ctrl_pid;

/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_PosEsti(void *argument) {
	Task_Param_t *task_param = (Task_Param_t*)argument;
	
	task_param->message_q.gimb_eulr = osMessageQueueNew(2, sizeof(AHRS_Eulr_t), NULL);
	
	bmi088.received_alert = osThreadGetId();
	BMI088_Init(&bmi088);
	
	ist8310.received_alert = osThreadGetId();
	IST8310_Init(&ist8310);
	
	IST8310_Receive(&ist8310);
	osThreadFlagsWait(IST8310_SIGNAL_MAGN_RAW_REDY, osFlagsWaitAll, osWaitForever);
	
	IST8310_Parse(&ist8310);
	
	AHRS_Init(&gimbal_ahrs, &ist8310.magn, BMI088_GetUpdateFreq(&bmi088));
	
	PID_Init(&imu_temp_ctrl_pid, PID_MODE_DERIVATIV_NONE, 1.f/BMI088_GetUpdateFreq(&bmi088));
	PID_SetParameters(&imu_temp_ctrl_pid, .005f, .001f, 0.f, 1.f, 1.f);
	
	BSP_PWM_Set(BSP_PWM_IMU_HEAT, 0.f);
	BSP_PWM_Start(BSP_PWM_IMU_HEAT);
	
	while(1) {
		/* Task body */
		osThreadFlagsWait(BMI088_SIGNAL_ACCL_NEW_DATA, osFlagsWaitAll, osWaitForever);
		BMI088_ReceiveAccl(&bmi088);
		osThreadFlagsWait(BMI088_SIGNAL_ACCL_RAW_REDY, osFlagsWaitAll, osWaitForever);
		
		osThreadFlagsWait(BMI088_SIGNAL_GYRO_NEW_DATA, osFlagsWaitAll, osWaitForever);
		BMI088_ReceiveGyro(&bmi088);
		osThreadFlagsWait(BMI088_SIGNAL_GYRO_RAW_REDY, osFlagsWaitAll, osWaitForever);
		
		
		if (osThreadFlagsWait(IST8310_SIGNAL_MAGN_NEW_DATA, osFlagsWaitAll, 0u) == IST8310_SIGNAL_MAGN_NEW_DATA) {
			IST8310_Receive(&ist8310);
			osThreadFlagsWait(IST8310_SIGNAL_MAGN_RAW_REDY, osFlagsWaitAll, 0u);
		}
		
		osKernelLock();
		BMI088_ParseAccl(&bmi088);
		BMI088_ParseGyro(&bmi088);
		IST8310_Parse(&ist8310);
		osKernelUnlock();
		
		uint32_t now = osKernelGetTickCount();
		AHRS_Update(&gimbal_ahrs, &bmi088.accl, &bmi088.gyro, &ist8310.magn);
		
		/* Debug */
		AHRS_GetEulr(&debug_eulr, &gimbal_ahrs);
		#if 0
		AHRS_Eulr_t *eulr_to_send = pvPortMalloc(sizeof(AHRS_Eulr_t));
		
		if (eulr_to_send) {
			AHRS_GetEulr(eulr_to_send, &gimbal_ahrs);
			
			const osStatus_t stat = osMessageQueuePut(task_param->message_q.gimb_eulr, eulr_to_send, 0, 0);
			
			if (stat == osErrorOS)
				vPortFree(eulr_to_send);
		}
		#endif
		BSP_PWM_Set(BSP_PWM_IMU_HEAT, PID_Calculate(&imu_temp_ctrl_pid, 50.f, bmi088.temp, 0.f, 0.f));
	}
}
