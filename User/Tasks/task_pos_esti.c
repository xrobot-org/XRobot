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
static const uint32_t delay_ms = 1000u / TASK_POSESTI_FREQ_HZ;


osPoolDef(ahrs_pool, 2, AHRS_Eulr_t);
osMessageQDef(ahrs_message, 2, AHRS_Eulr_t);

static IMU_t onboard_imu;
static AHRS_t gimbal_ahrs;
static PID_t imu_temp_ctrl_pid;

/* Runtime status. */
int stat_p_e = 0;
osStatus os_stat_p_e = osOK;
#if INCLUDE_uxTaskGetStackHighWaterMark
uint32_t task_pos_esti_stack;
#endif

/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_PosEsti(void const *argument) {
	Task_Param_t *task_param = (Task_Param_t*)argument;
	
	task_param->pool.ahrs = osPoolCreate(osPool(ahrs_pool));
	task_param->message.ahrs = osMessageCreate(osMessageQ(ahrs_message), NULL);
	
	/* Init IMU temp control. */
	stat_p_e += PID_Init(&imu_temp_ctrl_pid, PID_MODE_DERIVATIV_NONE, 1.f/TASK_POSESTI_FREQ_HZ);
	stat_p_e += PID_SetParameters(&imu_temp_ctrl_pid, .005f, .001f, 0.f, 1.f, 1.f);
	
	stat_p_e += BSP_PWM_Set(BSP_PWM_IMU_HEAT, 0.f);
	stat_p_e += BSP_PWM_Start(BSP_PWM_IMU_HEAT);
	
	
	/* Init IMU. */
	onboard_imu.received_alert = task_param->thread.pos_esti;
	stat_p_e += IMU_Init(&onboard_imu);
	
	stat_p_e += IMU_StartReceiving(&onboard_imu);
	
	/* Wait for new accl data. */
	osSignalWait(IMU_SIGNAL_RAW_ACCL_REDY, osWaitForever);
	stat_p_e += IMU_ParseAccl(&onboard_imu);
	
	/* Wait for new gyro data. */
	osSignalWait(IMU_SIGNAL_RAW_GYRO_REDY, osWaitForever);
	stat_p_e += IMU_ParseGyro(&onboard_imu);
	
	/* Try to get new magn data. */
	osSignalWait(COMP_SIGNAL_RAW_MAGN_REDY, 0u);
	//TODO: parse comp
	
	/* Init AHRS. */
	stat_p_e += AHRS_Init(&gimbal_ahrs, &onboard_imu, TASK_POSESTI_FREQ_HZ);
	
	
	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		/* Task body */
		
		/* Wait for new accl data. */
		osSignalWait(IMU_SIGNAL_RAW_ACCL_REDY, osWaitForever);
		stat_p_e += IMU_ParseAccl(&onboard_imu);
		
		/* Wait for new gyro data. */
		osSignalWait(IMU_SIGNAL_RAW_GYRO_REDY, osWaitForever);
		stat_p_e += IMU_ParseGyro(&onboard_imu);
		
		/* Try to get new magn data. */
		osSignalWait(COMP_SIGNAL_RAW_MAGN_REDY, 0u);
		//TODO: parse comp
		
		uint32_t now = osKernelSysTick();
		stat_p_e += AHRS_Update(&gimbal_ahrs, &onboard_imu);
		
		AHRS_Eulr_t *eulr_to_send;
		eulr_to_send = osPoolAlloc(task_param->pool.ahrs);
		memcpy(eulr_to_send, &(gimbal_ahrs.eulr), sizeof(AHRS_Eulr_t));
		
		/* Drop data if queue is full. */
		os_stat_p_e = osMessagePut(task_param->message.ahrs, (uint32_t)eulr_to_send, 1);
		
		/* Free memory if data dropped. */
		if (os_stat_p_e == osErrorOS)
			osPoolFree(task_param->pool.ahrs, eulr_to_send);

		stat_p_e += BSP_PWM_Set(BSP_PWM_IMU_HEAT, PID_Calculate(&imu_temp_ctrl_pid, 50.f, onboard_imu.data.temp, 0.f, 0.f));
		
		os_stat_p_e += osDelayUntil(&previous_wake_time, delay_ms);
		
#if INCLUDE_uxTaskGetStackHighWaterMark
        task_pos_esti_stack = uxTaskGetStackHighWaterMark(NULL);
#endif
	}
}
