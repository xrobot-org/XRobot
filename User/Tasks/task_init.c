#include "task_init.h"
#include "cmsis_os.h"

#include "io.h"
#include "oled.h"
#include "joystick.h"

#define INIT_TASK_STATUS_LED LED1

void InitTask(const void* argument) {

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
