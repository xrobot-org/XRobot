/* Includes ------------------------------------------------------------------*/
#include "bsp_gpio.h"

#include "main.h"
#include "gpio.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
struct {
	void(*ACCL_INT_Pin_Callback)(void);
	void(*GYRO_INT_Pin_Callback)(void);
	void(*USER_KEY_Pin_Callback)(void);
	void(*CMPS_INT_Pin_Callback)(void);
	/* void (*XXX_Pin_Callback)(void); */
}static bsp_gpio_callback;

/* Private function  ---------------------------------------------------------*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	switch (GPIO_Pin) {
		case ACCL_INT_Pin:
			if (bsp_gpio_callback.ACCL_INT_Pin_Callback)
				bsp_gpio_callback.ACCL_INT_Pin_Callback();
			break;
			
		case GYRO_INT_Pin:
			if (bsp_gpio_callback.GYRO_INT_Pin_Callback)
				bsp_gpio_callback.GYRO_INT_Pin_Callback();
			break;
			
		case USER_KEY_Pin:
			if (bsp_gpio_callback.USER_KEY_Pin_Callback)
				bsp_gpio_callback.USER_KEY_Pin_Callback();
			break;
			
		case CMPS_INT_Pin:
			if (bsp_gpio_callback.CMPS_INT_Pin_Callback)
				bsp_gpio_callback.CMPS_INT_Pin_Callback();
			break;	
		/*
		case XXX_Pin:
			if (bsp_gpio_callback.XXX_Pin_Callback)
				bsp_gpio_callback.XXX_Pin_Callback();
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
		
		case USER_KEY_Pin:
			bsp_gpio_callback.USER_KEY_Pin_Callback = callback;
			break;
		
		case CMPS_INT_Pin:
			bsp_gpio_callback.CMPS_INT_Pin_Callback = callback;
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

int BSP_GPIO_EnableIRQ(uint16_t pin) {
	switch (pin) {
		case ACCL_INT_Pin:
			HAL_NVIC_EnableIRQ(ACCL_INT_EXTI_IRQn);
			break;
		
		case GYRO_INT_Pin:
			HAL_NVIC_EnableIRQ(GYRO_INT_EXTI_IRQn);
			break;
		
		case USER_KEY_Pin:
			HAL_NVIC_EnableIRQ(USER_KEY_EXTI_IRQn);
			break;
		
		case CMPS_INT_Pin:
			HAL_NVIC_EnableIRQ(CMPS_INT_EXTI_IRQn);
			break;
		/*
		case XXX_Pin:
			HAL_NVIC_EnableIRQ(XXX_IRQn);
			break;
		*/
		
		default:
			return -1;
	}
	return 0;
}

int BSP_GPIO_DisableIRQ(uint16_t pin) {
	switch (pin) {
		case ACCL_INT_Pin:
			HAL_NVIC_DisableIRQ(ACCL_INT_EXTI_IRQn);
			break;
		
		case GYRO_INT_Pin:
			HAL_NVIC_DisableIRQ(GYRO_INT_EXTI_IRQn);
			break;
		
		case USER_KEY_Pin:
			HAL_NVIC_DisableIRQ(USER_KEY_EXTI_IRQn);
			break;
		
		case CMPS_INT_Pin:
			HAL_NVIC_DisableIRQ(CMPS_INT_EXTI_IRQn);
			break;
		/*
		case XXX_Pin:
			HAL_NVIC_EnableIRQ(XXX_IRQn);
			break;
		*/
		
		default:
			return -1;
	}
	return 0;
}
