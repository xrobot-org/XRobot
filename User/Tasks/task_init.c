#include "task_init.h"
#include "cmsis_os.h"

#include "io.h"
#include "oled.h"
#include "joystick.h"

#define INIT_TASK_STATUS_LED LED1

void InitTask(const void* argument) {
	uint32_t last_tick = osKernelSysTick();
	Joystick_Status_t js;
	
	LED_Set(INIT_TASK_STATUS_LED, LED_ON);
	
	OLED_Rewind();
	
	OLED_Print("Enter Debug?Y<>N\n");
	Joystick_WaitInput();
	
	osDelay(10);
	Joystick_Update(&js);
	
	if (js == JOYSTICK_LEFT) {
		OLED_Print("YES\n");
		OLED_Print("Cali Magnet?\n");
		
		Joystick_WaitNoInput();
		Joystick_WaitInput();
			
		osDelay(10);
		Joystick_Update(&js);
		
		if (js == JOYSTICK_LEFT) {
			OLED_Print("YES\n");
			for (int16_t i = 0; i < 1000; i++) {
				;
			}
		}
	} else {
		OLED_Print("NO\n");
	}
	
	LED_Set(INIT_TASK_STATUS_LED, LED_OFF);
	
	osThreadTerminate(osThreadGetId());
}
