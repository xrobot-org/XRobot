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
#include "robot_config.h"

/* Include Module相关的头文件 */
#include "shoot.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static CAN_Device_t *cd;
static DR16_t *dr16;

static Shoot_t shoot;
static Shoot_Ctrl_t shoot_ctrl;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_CtrlShoot(void *argument) {
	const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_CTRL_SHOOT;
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_CTRL_SHOOT);
	
	cd = CAN_GetDevice();
	dr16 = DR16_GetDevice();
	
	Shoot_Init(&shoot, &(RobotConfig_Get(ROBOT_CONFIG_MODEL_INFANTRY)->shoot_param));
	shoot.dt_sec = (float)delay_tick / (float)osKernelGetTickFreq();
	
	uint32_t tick = osKernelGetTickCount();
	while(1) {
		/* Task body */
		tick += delay_tick;
		
		osThreadFlagsWait(DR16_SIGNAL_DATA_REDY, osFlagsWaitAll, 0);
		Shoot_ParseCommand(&shoot_ctrl, dr16);
		
		if (osThreadFlagsWait(CAN_DEVICE_SIGNAL_MOTOR_RECV, osFlagsWaitAll, delay_tick) != osFlagsErrorTimeout) {
			
			osKernelLock();
			Shoot_UpdateFeedback(&shoot, cd);
			osKernelUnlock();
			
			Shoot_SetMode(&shoot, shoot_ctrl.mode);
			Shoot_Control(&shoot, shoot_ctrl.bullet_speed, shoot_ctrl.shoot_freq_hz);
			
			// TODO: Check can error.
			CAN_Motor_ControlShoot(
				shoot.fric_cur_out[0],
				shoot.fric_cur_out[1],
				shoot.trig_cur_out
			);
			
			osDelayUntil(tick);
		} else {
			CAN_Motor_ControlShoot(0.f, 0.f, 0.f);
		}
	}
}
