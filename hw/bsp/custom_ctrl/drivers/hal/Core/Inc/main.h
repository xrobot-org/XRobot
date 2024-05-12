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
#include "stm32f1xx_hal.h"

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
void MX_SPI1_Init(void);
void MX_CAN_Init(void);
void MX_TIM2_Init(void);
void MX_USART3_UART_Init(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_Pin GPIO_PIN_13
#define LED_GPIO_Port GPIOC
#define GYRO_CS_Pin GPIO_PIN_3
#define GYRO_CS_GPIO_Port GPIOA
#define ACCL_CS_Pin GPIO_PIN_4
#define ACCL_CS_GPIO_Port GPIOA
#define GYRO_INT_Pin GPIO_PIN_0
#define GYRO_INT_GPIO_Port GPIOB
#define GYRO_INT_EXTI_IRQn EXTI0_IRQn
#define ACCL_INT_Pin GPIO_PIN_1
#define ACCL_INT_GPIO_Port GPIOB
#define ACCL_INT_EXTI_IRQn EXTI1_IRQn
#define I2C3_SDA_Pin GPIO_PIN_12
#define I2C3_SDA_GPIO_Port GPIOB
#define I2C3_SCL_Pin GPIO_PIN_13
#define I2C3_SCL_GPIO_Port GPIOB
#define I2C4_SDA_Pin GPIO_PIN_14
#define I2C4_SDA_GPIO_Port GPIOB
#define I2C4_SCL_Pin GPIO_PIN_15
#define I2C4_SCL_GPIO_Port GPIOB
#define I2C5_SDA_Pin GPIO_PIN_8
#define I2C5_SDA_GPIO_Port GPIOA
#define I2C5_SCL_Pin GPIO_PIN_9
#define I2C5_SCL_GPIO_Port GPIOA
#define I2C6_SDA_Pin GPIO_PIN_10
#define I2C6_SDA_GPIO_Port GPIOA
#define I2C6_SCL_Pin GPIO_PIN_11
#define I2C6_SCL_GPIO_Port GPIOA
#define I2C2_SDA_Pin GPIO_PIN_4
#define I2C2_SDA_GPIO_Port GPIOB
#define I2C2_SCL_Pin GPIO_PIN_5
#define I2C2_SCL_GPIO_Port GPIOB
#define I2C1_SDA_Pin GPIO_PIN_6
#define I2C1_SDA_GPIO_Port GPIOB
#define I2C1_SCL_Pin GPIO_PIN_7
#define I2C1_SCL_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
void SystemClock_Config(void);
void HAL_MspInit(void);
void assert_failed(uint8_t *file, uint32_t line);
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
