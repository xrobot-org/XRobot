#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Include cmsis_os.h头文件 */
#include "cmsis_os.h"

#include "ahrs.h"

/* Exported constants --------------------------------------------------------*/
#define IMU_OK			(0)
#define IMU_ERR			(-1)
#define IMU_ERR_NULL	(-2)
#define IMU_ERR_INITED	(-3)
#define IMU_ERR_NO_DEV	(-4)

#define IMU_SIGNAL_RAW_ACCL_REDY	(1u<<7)
#define IMU_SIGNAL_RAW_GYRO_REDY	(1u<<8)

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct {
	osThreadId received_alert;

	uint8_t raw[20];
	AHRS_Accl_t accl;
	AHRS_Gyro_t gyro;
	
	float temp;
	
	struct {
		int gyro_offset[3];
		int magn_offset[3];
		int magn_scale[3];
	} cali;
} IMU_t;

/* Exported functions prototypes ---------------------------------------------*/
int IMU_Init(IMU_t *imu);
IMU_t *IMU_GetDevice(void);

int IMU_StartReceiving(IMU_t *imu);

/* Sensor use right-handed coordinate system. */
/*         
		x < R(logo)
		    y
		UP is z
	All implementation should follow this rule.
 */
int IMU_ParseAccl(IMU_t *imu);
int IMU_ParseGyro(IMU_t *imu);


int IMU_Restart(void);
