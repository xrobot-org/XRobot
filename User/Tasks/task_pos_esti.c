/*
	姿态解算任务。
	
	控制IMU加热到指定温度防止温漂，收集IMU数据给AHRS算法，
	解算后的数据发送给需要用的任务。
*/

/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

#include "imu.h"
#include "io.h"
#include "pwm.h"

#include "oled.h"

#include "ahrs.h"
#include "pid.h"

#define TASK_POSESTI_FREQ_HZ (50)
#define TASK_POSESTI_INIT_DELAY (500)

void Task_PosEsti(const void* argument) {
	uint32_t delay_tick = 1000U / TASK_POSESTI_FREQ_HZ;
	
	IMU_t imu;
	AHRS_t gimbal_ahrs;
	PID_t imu_temp_ctrl_pid;
	
	float duty_cycle = 0.f;
	
	IMU_Init(&imu);
	IMU_Update(&imu);
	AHRS_Init(&gimbal_ahrs, &imu, TASK_POSESTI_FREQ_HZ);
	PID_Init(&imu_temp_ctrl_pid, PID_MODE_DERIVATIV_NONE, 1.f/TASK_POSESTI_FREQ_HZ);
	PID_SetParameters(&imu_temp_ctrl_pid, 5.f, 1.f, 0.f, PWM_RESOLUTION, PWM_RESOLUTION);
	
	osDelay(TASK_POSESTI_INIT_DELAY);
	while(1) {
		IMU_Update(&imu);
		AHRS_Update(&gimbal_ahrs, &imu);
		
		//ready to send data to a list of task;
		//they only use data, never change it;
		//use simple alert or copy and send all data;
		
		PID_Calculate(&imu_temp_ctrl_pid, 50.f, imu.data.temp, 0.f, 1.f/TASK_POSESTI_FREQ_HZ);
		PWM_Set(PWM_IMU_HEAT, duty_cycle);
		
		osDelayUntil(delay_tick);
	}
}
