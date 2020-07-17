#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Include cmsis_os2.h头文件 */
#include "cmsis_os2.h"

#include "component\ahrs.h"

/* Exported constants --------------------------------------------------------*/
#define BMI088_OK			(0)
#define BMI088_ERR			(-1)
#define BMI088_ERR_NULL		(-2)
#define BMI088_ERR_INITED	(-3)
#define BMI088_ERR_NO_DEV	(-4)

#define BMI088_SIGNAL_GYRO_NEW_DATA	(1u<<0)
#define BMI088_SIGNAL_ACCL_NEW_DATA	(1u<<1)
#define BMI088_SIGNAL_GYRO_RAW_REDY	(1u<<2)
#define BMI088_SIGNAL_ACCL_RAW_REDY	(1u<<3)

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct {
	osThreadId_t thread_alert;

	AHRS_Accl_t accl;
	AHRS_Gyro_t gyro;
	
	float32_t temp;
	
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
float32_t BMI088_GetUpdateFreq(BMI088_t *bmi088);
