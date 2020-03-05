#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Include cmsis_os.h头文件 */
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"

#include "ahrs.h"

/* Exported constants --------------------------------------------------------*/
#define BMI088_OK			(0)
#define BMI088_ERR			(-1)
#define BMI088_ERR_NULL	(-2)
#define BMI088_ERR_INITED	(-3)
#define BMI088_ERR_NO_DEV	(-4)

#define BMI088_SIGNAL_GYRO_NEW_DATA	(1u<<0)
#define BMI088_SIGNAL_ACCL_NEW_DATA	(1u<<1)
#define BMI088_SIGNAL_GYRO_RAW_REDY	(1u<<2)
#define BMI088_SIGNAL_ACCL_RAW_REDY	(1u<<3)

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct {
	osThreadId_t received_alert;

	uint8_t raw[25];
	AHRS_Accl_t accl;
	AHRS_Gyro_t gyro;
	
	float temp;
	
	struct {
		int gyro_offset[3];
	} cali;
} BMI088_t;

/* Exported functions prototypes ---------------------------------------------*/
int BMI088_Init(BMI088_t *bmi088);
BMI088_t *BMI088_GetDevice(void);
int BMI088_Restart(void);

/* Sensor use right-handed coordinate system. */
/*		 
		x < R(logo)
			y
		UP is z
	All implementation should follow this rule.
 */
int BMI088_ReceiveAccl(BMI088_t *bmi088);
int BMI088_ReceiveGyro(BMI088_t *bmi088);
int BMI088_ParseAccl(BMI088_t *bmi088);
int BMI088_ParseGyro(BMI088_t *bmi088);
float BMI088_GetUpdateFreq(BMI088_t *bmi088);
