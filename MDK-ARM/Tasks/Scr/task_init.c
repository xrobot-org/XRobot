#include "task_init.h"
#include "main.h"
#include "cmsis_os.h"

#include "bsp_io.h"
#include "bsp_oled.h"

#define INIT_TASK_STATUS_LED LED7

void InitTask(const void* argument) {
	uint32_t last_tick = osKernelSysTick();
	Joystick_StatusTypedef js;
	
	LED_Set(INIT_TASK_STATUS_LED, LED_ON);
	
	OLED_Rewind();
	
	OLED_Print("YES<-\nEnter Debug?\n");
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
			}
		}
	} else {
		OLED_Print("NO\n");
	}
	
	LED_Set(INIT_TASK_STATUS_LED, LED_OFF);
	
	osThreadTerminate(osThreadGetId());
}
