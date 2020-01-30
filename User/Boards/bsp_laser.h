#pragma once


/* Includes ------------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/

/* duty_cycle大于零时，A板为全开，C板为pwm调光*/
int BSP_Laser_Start(void);
int BSP_Laser_Set(float duty_cycle);
int BSP_Laser_Stop(void);
