#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Include cmsis_os2.h头文件 */
#include "cmsis_os2.h"

#include "device.h"

#include "component\ahrs.h"

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct {
	osThreadId_t thread_alert;

	AHRS_Accl_t accl;
	AHRS_Gyro_t gyro;
	
	float temp;
	
	struct {
		int8_t gyro_offset[3];
	} cali;
} BMI088_t;

/* Exported functions prototypes ---------------------------------------------*/
int8_t BMI088_Init(BMI088_t *bmi088, osThreadId_t thread_alert);
BMI088_t *BMI088_GetDevice(void);
int8_t BMI088_Restart(void);

/* Sensor use right-handed coordinate system. */
/*		 
		x < R(logo)
			y
		UP is z
	All implementation should follow this rule.
 */
int8_t BMI088_ReceiveAccl(BMI088_t *bmi088);
int8_t BMI088_ReceiveGyro(BMI088_t *bmi088);
int8_t BMI088_ParseAccl(BMI088_t *bmi088);
int8_t BMI088_ParseGyro(BMI088_t *bmi088);
float BMI088_GetUpdateFreq(BMI088_t *bmi088);
