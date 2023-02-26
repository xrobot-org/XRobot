/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
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

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
void   MX_GPIO_Init(void);
void   MX_DMA_Init(void);
void   MX_CAN1_Init(void);
void   MX_UART4_Init(void);
void   MX_CAN2_Init(void);
void   MX_SPI1_Init(void);
void   MX_TIM3_Init(void);
void   MX_UART5_Init(void);
void   MX_USART1_UART_Init(void);
void   MX_USART2_UART_Init(void);
void   MX_USART3_UART_Init(void);
void   MX_USART6_UART_Init(void);
void   MX_USB_OTG_FS_PCD_Init(void);
void   MX_IWDG_Init(void);
void   MX_TIM14_Init(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ACCL_CS_Pin GPIO_PIN_4
#define ACCL_CS_GPIO_Port GPIOA
#define GYRO_CS_Pin GPIO_PIN_4
#define GYRO_CS_GPIO_Port GPIOC
#define ACCL_INT_Pin GPIO_PIN_5
#define ACCL_INT_GPIO_Port GPIOC
#define ACCL_INT_EXTI_IRQn EXTI9_5_IRQn
#define GYRO_INT_Pin GPIO_PIN_0
#define GYRO_INT_GPIO_Port GPIOB
#define GYRO_INT_EXTI_IRQn EXTI0_IRQn
#define LED_Pin GPIO_PIN_9
#define LED_GPIO_Port GPIOC

/* USER CODE BEGIN Private defines */
void SystemClock_Config(void);
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
