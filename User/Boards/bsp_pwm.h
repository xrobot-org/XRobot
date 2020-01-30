#pragma once


/* Includes ------------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum {
	BSP_PWM_IMU_HEAT,
} BSP_PWM_Channel_t;

/* Exported functions prototypes ---------------------------------------------*/
int BSP_PWM_Start(BSP_PWM_Channel_t ch);
int BSP_PWM_Set(BSP_PWM_Channel_t ch, float duty_cycle);
int BSP_PWM_Stop(BSP_PWM_Channel_t ch);
