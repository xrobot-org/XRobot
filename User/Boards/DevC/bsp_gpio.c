/* Includes ------------------------------------------------------------------*/
#include "bsp_gpio.h"
#include "main.h"
#include "gpio.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static struct {
	void (*ACCL_INT_Pin_Callback)(void);
	void (*GYRO_INT_Pin_Callback)(void);
	/* void (*XXX_Pin_Callback)(void); */
} bsp_gpio_callback;

/* Private function  ---------------------------------------------------------*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	switch (GPIO_Pin) {
		case ACCL_INT_Pin:
			if (bsp_gpio_callback.ACCL_INT_Pin_Callback != NULL) {
				bsp_gpio_callback.ACCL_INT_Pin_Callback();
			}
			break;
			
		case GYRO_INT_Pin:
			if (bsp_gpio_callback.GYRO_INT_Pin_Callback != NULL) {
				bsp_gpio_callback.GYRO_INT_Pin_Callback();
			}
			break;
		
		/*
		case XXX_Pin:
			if (bsp_gpio_callback.XXX_Pin_Callback != NULL) {
				bsp_gpio_callback.XXX_Pin_Callback();
			}
			break;
		*/
		
		default:
			return;
			
	}
}

/* Exported functions --------------------------------------------------------*/
int BSP_GPIO_RegisterCallback(uint16_t pin, void (*callback)(void)) {
	if (callback == NULL)
		return -1;
	
    switch (pin) {
		case ACCL_INT_Pin:
			bsp_gpio_callback.ACCL_INT_Pin_Callback = callback;
			break;
		
		case GYRO_INT_Pin:
			bsp_gpio_callback.GYRO_INT_Pin_Callback = callback;
			break;
		
		/*
		case XXX_Pin:
			bsp_gpio_callback.XXX_Pin_Callback = callback;
			break;
		*/
		
		default:
			return -1;
	}
    return 0;
}
