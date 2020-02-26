/*
	控制射击。

*/

/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include 标准库 */
/* Include Board相关的头文件 */
#include "bsp_usb.h"

/* Include Device相关的头文件 */
/* Include Component相关的头文件 */
#include "shoot.h"

/* Include Module相关的头文件 */
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static const uint32_t delay_ms = osKernelSysTickFrequency / TASK_FREQ_HZ_CTRL_SHOOT;

static CAN_Device_t *cd;
static DR16_t *dr16;

static Shoot_t shoot;
static Shoot_Ctrl_t shoot_ctrl;

/* Runtime status. */
int stat_c_s = 0;
osStatus os_stat_c_s = osOK;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_CtrlShoot(void const *argument) {
	Task_Param_t *task_param = (Task_Param_t*)argument;
	
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_CTRL_SHOOT);
	
	cd = CAN_GetDevice();
	dr16 = DR16_GetDevice();

	Shoot_Init(&shoot);
	shoot.dt_sec = (float)delay_ms / 1000.f;
	
	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		/* Task body */
		
		/* Try to get new rc command. */
		osSignalWait(DR16_SIGNAL_DATA_REDY, 0);
		Shoot_ParseCommand(&shoot_ctrl, dr16);
		
		/* Wait for motor feedback. */
		osSignalWait(CAN_DEVICE_SIGNAL_MOTOR_RECV, osWaitForever);
		
		taskENTER_CRITICAL();
		Shoot_UpdateFeedback(&shoot, cd);
		taskEXIT_CRITICAL();
		
		Shoot_SetMode(&shoot, shoot_ctrl.mode);
		Shoot_Control(&shoot, shoot_ctrl.bullet_speed, shoot_ctrl.shoot_freq_hz);
		
		// Check can error
		CAN_Motor_ControlShoot(
			shoot.fric_cur_out[0],
			shoot.fric_cur_out[1],
			shoot.trig_cur_out
		);
		
		osDelayUntil(&previous_wake_time, delay_ms);
	}
}
