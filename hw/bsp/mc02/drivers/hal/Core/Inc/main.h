/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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
#include "stm32h7xx_hal.h"

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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
void MX_GPIO_Init(void);
void MX_DMA_Init(void);
void MX_ADC1_Init(void);
void MX_TIM12_Init(void);
void MX_SPI2_Init(void);
void MX_TIM3_Init(void);
void MX_FDCAN1_Init(void);
void MX_FDCAN2_Init(void);
void MX_FDCAN3_Init(void);
void MX_TIM1_Init(void);
void MX_TIM2_Init(void);
void MX_OCTOSPI2_Init(void);
void MX_USB_OTG_HS_PCD_Init(void);
void MX_TIM7_Init(void);
void MX_SPI6_Init(void);
void MX_UART5_Init(void);
void MX_USART3_UART_Init(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define POWER_24V_2_Pin GPIO_PIN_13
#define POWER_24V_2_GPIO_Port GPIOC
#define POWER_5V_Pin GPIO_PIN_15
#define POWER_5V_GPIO_Port GPIOC
#define ACCL_CS_Pin GPIO_PIN_0
#define ACCL_CS_GPIO_Port GPIOC
#define GYRO_CS_Pin GPIO_PIN_3
#define GYRO_CS_GPIO_Port GPIOC
#define DCMI_PWDN_Pin GPIO_PIN_5
#define DCMI_PWDN_GPIO_Port GPIOC
#define ACCL_INT_Pin GPIO_PIN_10
#define ACCL_INT_GPIO_Port GPIOE
#define ACCL_INT_EXTI_IRQn EXTI15_10_IRQn
#define GYRO_INT_Pin GPIO_PIN_12
#define GYRO_INT_GPIO_Port GPIOE
#define GYRO_INT_EXTI_IRQn EXTI15_10_IRQn
#define LCD_CS_Pin GPIO_PIN_15
#define LCD_CS_GPIO_Port GPIOE
#define LCD_BLK_Pin GPIO_PIN_10
#define LCD_BLK_GPIO_Port GPIOB
#define LCD_RES_Pin GPIO_PIN_11
#define LCD_RES_GPIO_Port GPIOB
#define DCMI_REST_Pin GPIO_PIN_12
#define DCMI_REST_GPIO_Port GPIOB
#define LCD_DC_Pin GPIO_PIN_10
#define LCD_DC_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */
void SystemClock_Config(void);
void HAL_RealtimeClockStart(void);
unsigned long HAL_RealtimeClockGetValue(void);
void PeriphCommonClock_Config(void);
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
