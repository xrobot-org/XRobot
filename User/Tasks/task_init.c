/*
	初始化任务，用于启动所有任务。
	
	
*/


/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include Board相关的头文件。*/
/* Include Device相关的头文件。*/
/* Include Component相关的头文件。*/
/* Include Module相关的头文件。*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
Task_List_t task_list;

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
