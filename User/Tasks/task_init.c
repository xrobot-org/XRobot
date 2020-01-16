/*
	初始化任务，用于启动所有任务。
	
	
*/


/* Includes ------------------------------------------------------------------*/
/* Include 自身的头文件，OS头文件。*/
#include "task_init.h"
#include "cmsis_os2.h"

/* Include Board相关的头文件。*/
#include "io.h"

/* Include Device相关的头文件。*/
/* Include Component相关的头文件。*/
/* Include Module相关的头文件。*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define TASK_DEBUG_FREQ_HZ (50)
#define TASK_DEBUG_INIT_DELAY (500)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

void Task_Init(const void* argument) {
	
	
	
	#if defined ROBOT_TYPE_INFANTRY
		
	#elif defined ROBOT_TYPE_HERO
		
	#elif defined ROBOT_TYPE_ENGINEER
		
	#elif defined ROBOT_TYPE_DRONE
		
	#elif defined ROBOT_TYPE_SENTRY

	#else
		
		#error: Must define ROBOT_TYPE_XXXX.
		
	#endif
		
	osThreadTerminate(osThreadGetId());
	
}