/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
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
extern TIM_HandleTypeDef htim14;
/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
void MX_GPIO_Init(void);
void MX_DMA_Init(void);
void MX_ADC1_Init(void);
void MX_ADC3_Init(void);
void MX_CAN1_Init(void);
void MX_CAN2_Init(void);
void MX_I2C1_Init(void);
void MX_SPI1_Init(void);
void MX_TIM4_Init(void);
void MX_TIM5_Init(void);
void MX_USART3_UART_Init(void);
void MX_TIM8_Init(void);
void MX_RNG_Init(void);
void MX_I2C2_Init(void);
void MX_I2C3_Init(void);
void MX_TIM1_Init(void);
void MX_TIM3_Init(void);
void MX_TIM10_Init(void);
void MX_USART1_UART_Init(void);
void MX_USART6_UART_Init(void);
void MX_TIM7_Init(void);
void MX_USB_OTG_FS_PCD_Init(void);
void MX_IWDG_Init(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LASER_Pin GPIO_PIN_8
#define LASER_GPIO_Port GPIOC
#define CMPS_RST_Pin GPIO_PIN_6
#define CMPS_RST_GPIO_Port GPIOG
#define IMU_HEAT_PWM_Pin GPIO_PIN_6
#define IMU_HEAT_PWM_GPIO_Port GPIOF
#define LED_R_Pin GPIO_PIN_12
#define LED_R_GPIO_Port GPIOH
#define CMPS_INT_Pin GPIO_PIN_3
#define CMPS_INT_GPIO_Port GPIOG
#define CMPS_INT_EXTI_IRQn EXTI3_IRQn
#define ADC_BAT_Pin GPIO_PIN_10
#define ADC_BAT_GPIO_Port GPIOF
#define LED_G_Pin GPIO_PIN_11
#define LED_G_GPIO_Port GPIOH
#define LED_B_Pin GPIO_PIN_10
#define LED_B_GPIO_Port GPIOH
#define HW0_Pin GPIO_PIN_0
#define HW0_GPIO_Port GPIOC
#define HW1_Pin GPIO_PIN_1
#define HW1_GPIO_Port GPIOC
#define HW2_Pin GPIO_PIN_2
#define HW2_GPIO_Port GPIOC
#define BUZZER_Pin GPIO_PIN_14
#define BUZZER_GPIO_Port GPIOD
#define USER_KEY_Pin GPIO_PIN_0
#define USER_KEY_GPIO_Port GPIOA
#define USER_KEY_EXTI_IRQn EXTI0_IRQn
#define ACCL_CS_Pin GPIO_PIN_4
#define ACCL_CS_GPIO_Port GPIOA
#define ACCL_INT_Pin GPIO_PIN_4
#define ACCL_INT_GPIO_Port GPIOC
#define ACCL_INT_EXTI_IRQn EXTI4_IRQn
#define GYRO_INT_Pin GPIO_PIN_5
#define GYRO_INT_GPIO_Port GPIOC
#define GYRO_INT_EXTI_IRQn EXTI9_5_IRQn
#define SWITCH_Pin GPIO_PIN_12
#define SWITCH_GPIO_Port GPIOB
#define GYRO_CS_Pin GPIO_PIN_0
#define GYRO_CS_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
void SystemClock_Config(void);
void HAL_RealtimeClockStart(void);
unsigned long HAL_RealtimeClockGetValue(void);
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
