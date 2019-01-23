/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2018 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define APB2_TIMER_CLOCKS 168000000
#define APB1_TIMER_CLOCKS 84000000
#define PWM_RESOLUTION 10000
#define PWM_FREQUENCE 50
#define PWM_DEFAULT_DUTY 5000
#define TIM_PSC_APB1 ((APB1_TIMER_CLOCKS/PWM_FREQUENCE)/PWM_RESOLUTION -1)
#define TIM_PSC_APB2 ((APB2_TIMER_CLOCKS/PWM_FREQUENCE)/PWM_RESOLUTION -1)
#define BUZZER_FREQUENCE 2731
#define BUZZER_TIM_PSC_APB2 ((APB2_TIMER_CLOCKS/BUZZER_FREQUENCE)/PWM_RESOLUTION -1)
#define CMPS_INT_Pin GPIO_PIN_3
#define CMPS_INT_GPIO_Port GPIOE
#define CMPS_RST_Pin GPIO_PIN_2
#define CMPS_RST_GPIO_Port GPIOE
#define IMU_INT_Pin GPIO_PIN_8
#define IMU_INT_GPIO_Port GPIOB
#define HEAT_PWM_Pin GPIO_PIN_5
#define HEAT_PWM_GPIO_Port GPIOB
#define LASER_Pin GPIO_PIN_13
#define LASER_GPIO_Port GPIOG
#define OLED_DC_Pin GPIO_PIN_9
#define OLED_DC_GPIO_Port GPIOB
#define POWER1_CTRL_Pin GPIO_PIN_2
#define POWER1_CTRL_GPIO_Port GPIOH
#define POWER2_CTRL_Pin GPIO_PIN_3
#define POWER2_CTRL_GPIO_Port GPIOH
#define POWER3_CTRL_Pin GPIO_PIN_4
#define POWER3_CTRL_GPIO_Port GPIOH
#define LED8_Pin GPIO_PIN_8
#define LED8_GPIO_Port GPIOG
#define POWER4_CTRL_Pin GPIO_PIN_5
#define POWER4_CTRL_GPIO_Port GPIOH
#define LED7_Pin GPIO_PIN_7
#define LED7_GPIO_Port GPIOG
#define LED6_Pin GPIO_PIN_6
#define LED6_GPIO_Port GPIOG
#define SPI5_NSS_Pin GPIO_PIN_6
#define SPI5_NSS_GPIO_Port GPIOF
#define LED5_Pin GPIO_PIN_5
#define LED5_GPIO_Port GPIOG
#define LED4_Pin GPIO_PIN_4
#define LED4_GPIO_Port GPIOG
#define LED3_Pin GPIO_PIN_3
#define LED3_GPIO_Port GPIOG
#define LED2_Pin GPIO_PIN_2
#define LED2_GPIO_Port GPIOG
#define USER_KEY_Pin GPIO_PIN_2
#define USER_KEY_GPIO_Port GPIOB
#define LED1_Pin GPIO_PIN_1
#define LED1_GPIO_Port GPIOG
#define BUZZER_Pin GPIO_PIN_6
#define BUZZER_GPIO_Port GPIOH
#define BUTTON_AD_Pin GPIO_PIN_6
#define BUTTON_AD_GPIO_Port GPIOA
#define LED_RED_Pin GPIO_PIN_11
#define LED_RED_GPIO_Port GPIOE
#define LED_GRN_Pin GPIO_PIN_14
#define LED_GRN_GPIO_Port GPIOF
#define SD_EXTI_Pin GPIO_PIN_15
#define SD_EXTI_GPIO_Port GPIOE
#define OLED_RST_Pin GPIO_PIN_10
#define OLED_RST_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
