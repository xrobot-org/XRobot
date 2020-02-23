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
#include "imu.h"
#include "compass.h"

/* Include Component相关的头文件 */
#include "ahrs.h"
#include "pid.h"

/* Include Module相关的头文件 */
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static const uint32_t delay_ms = osKernelSysTickFrequency / TASK_FREQ_HZ_POSESTI;

static osMessageQDef(gimb_eulr_message, 2, AHRS_Eulr_t);

static IMU_t imu;
static AHRS_t gimbal_ahrs;
static PID_t imu_temp_ctrl_pid;

static osStatus os_status = osOK;

/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_PosEsti(void const *argument) {
	Task_Param_t *task_param = (Task_Param_t*)argument;
	
	task_param->message.gimb_eulr = osMessageCreate(osMessageQ(gimb_eulr_message), NULL);
	
	/* Init IMU temp control. */
	PID_Init(&imu_temp_ctrl_pid, PID_MODE_DERIVATIV_NONE, 1.f/TASK_FREQ_HZ_POSESTI);
	PID_SetParameters(&imu_temp_ctrl_pid, .005f, .001f, 0.f, 1.f, 1.f);
	
	BSP_PWM_Set(BSP_PWM_IMU_HEAT, 0.f);
	BSP_PWM_Start(BSP_PWM_IMU_HEAT);
	
	
	/* Init IMU. */
	imu.received_alert = osThreadGetId();
	IMU_Init(&imu);
	
	IMU_StartReceiving(&imu);
	
	/* Wait for new accl data. */
	osSignalWait(IMU_SIGNAL_RAW_ACCL_REDY, osWaitForever);
	IMU_ParseAccl(&imu);
	
	/* Wait for new gyro data. */
	osSignalWait(IMU_SIGNAL_RAW_GYRO_REDY, osWaitForever);
	IMU_ParseGyro(&imu);
	
	/* Try to get new magn data. */
	//osSignalWait(COMP_SIGNAL_RAW_MAGN_REDY, 0u);
	//TODO: parse comp
	
	/* Init AHRS. */
	AHRS_Init(&gimbal_ahrs, &imu.accl, &imu.gyro, NULL, TASK_FREQ_HZ_POSESTI);
	
	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		/* Task body */
		
		/* Wait for new accl data. */
		osSignalWait(IMU_SIGNAL_RAW_ACCL_REDY, osWaitForever);
		IMU_ParseAccl(&imu);
		
		/* Wait for new gyro data. */
		osSignalWait(IMU_SIGNAL_RAW_GYRO_REDY, osWaitForever);
		IMU_ParseGyro(&imu);
		
		/* Try to get new magn data. */
		osSignalWait(COMP_SIGNAL_RAW_MAGN_REDY, 0u);
		//TODO: parse comp
		
		uint32_t now = osKernelSysTick();
		AHRS_Update(&gimbal_ahrs, &imu.accl, &imu.gyro, NULL);
		
		/* Copy to new memory then send. */
		AHRS_Eulr_t *eulr_to_send = pvPortMalloc(sizeof(AHRS_Eulr_t));
		
		if (eulr_to_send) {
			memcpy(eulr_to_send, &(gimbal_ahrs.eulr), sizeof(AHRS_Eulr_t));
		
			/* Drop data if queue is full. */
			os_status = osMessagePut(task_param->message.gimb_eulr, (uint32_t)eulr_to_send, 0);
			
			/* Free memory if data dropped. */
			if (os_status == osErrorOS)
				vPortFree(eulr_to_send);
		}
		
		
		BSP_PWM_Set(BSP_PWM_IMU_HEAT, PID_Calculate(&imu_temp_ctrl_pid, 50.f, imu.temp, 0.f, 0.f));
		
		osDelayUntil(&previous_wake_time, delay_ms);
	}
}
