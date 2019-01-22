#include "task_imu.h"
#include "main.h"
#include "cmsis_os.h"

#include "bsp_imu.h"
#include "bsp_io.h"
#include "bsp_oled.h"

#include "tool_ahrs.h"
#include "tool_pid.h"

#define AHRS_TASK_FREQ_HZ (100)
#define AHRS_TASK_STATUS_LED LED6
 
IMU_HandleTypeDef himu = {0};

/* Add mutex? */
AHRS_HandleTypeDef hahrs = {0};
PID_HandleTypeDef imu_temp_ctrl_pid = {0};

void IMUTask(const void* argument) {
	uint32_t last_tick = osKernelSysTick();
	float duty_cycle = 0.f;
	
	LED_Set(AHRS_TASK_STATUS_LED, LED_ON);
	
	IMU_Update(&himu);
	AHRS_Init(&hahrs, &himu, AHRS_TASK_FREQ_HZ);
	PID_Init(&imu_temp_ctrl_pid, 5.f, 1.f, 0.f, 1.f);
	
	while(1) {
		/* Calculate attitude. */
		IMU_Update(&himu);
		AHRS_Update(&hahrs, &himu);
		
		/* Heat IMU to 50C. */
		PID_Update(&imu_temp_ctrl_pid, 50.f, himu.temp, &duty_cycle);
		PWM_Set(PWM_IMU_HEAT, duty_cycle);
		
		LED_Set(AHRS_TASK_STATUS_LED, LED_TAGGLE);
		osDelayUntil(&last_tick, (1000 / AHRS_TASK_FREQ_HZ));
	}
}
