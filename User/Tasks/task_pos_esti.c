#include "task_pos_esti.h"
#include "cmsis_os.h"

#include "imu.h"
#include "io.h"
#include "pwm.h"
#include "oled.h"

#include "ahrs.h"
#include "pid.h"

#define AHRS_TASK_FREQ_HZ (500)
#define AHRS_TASK_STATUS_LED LED6
 
void Task_PosEsti(const void* argument) {
	IMU_t imu = {0};
	AHRS_t gimbal_ahrs = {0};
	PID_t imu_temp_ctrl_pid = {0};
	
	float duty_cycle = 0.f;

	LED_Set(AHRS_TASK_STATUS_LED, LED_ON);
	
	IMU_Init();
	IMU_Update(&imu);
	AHRS_Init(&gimbal_ahrs, &imu, AHRS_TASK_FREQ_HZ);
	PID_Init(&imu_temp_ctrl_pid, 5.f, 1.f, 0.f, 1.f);
	
	while(1) {
		IMU_Update(&imu);
		AHRS_Update(&gimbal_ahrs, &imu);
		
		//ready to send data to a list of task;
		//they only use data, never change it;
		//use simple alert or copy and send all data;
		
		PID_Update(&imu_temp_ctrl_pid, 50.f, imu.data.temp, &duty_cycle);
		PWM_Set(PWM_IMU_HEAT, duty_cycle);
		
		LED_Set(AHRS_TASK_STATUS_LED, LED_TAGGLE);
		osDelayUntil(1000 / AHRS_TASK_FREQ_HZ);
	}
}
