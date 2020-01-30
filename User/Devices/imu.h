#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Include cmsis_os.h头文件 */
#include "cmsis_os.h"

/* Exported constants --------------------------------------------------------*/
#define IMU_SIGNAL_DATA_REDY (1u<<0)

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct {
	osThreadId received_alert;

	__packed struct {
		__packed struct {
			int16_t x;
			int16_t y;
			int16_t z;
		} accl;
		
		int16_t temp;
		
		__packed struct {
			int16_t x;
			int16_t y;
			int16_t z;
		} gyro;
		
		__packed struct {
			int16_t x;
			int16_t y;
			int16_t z;
		} magn;
	} raw;

	struct {
		struct {
			float x;
			float y;
			float z;
		} accl;
		
		struct {
			float x;
			float y;
			float z;
		} gyro;
		
		struct {
			float x;
			float y;
			float z;
		} magn;
		
		float temp;
	} data;
	
	struct {
		float gyro_offset[3];
		float magn_offset[3];
		float magn_scale[3];
	} cali;
} IMU_t;

/* Exported functions prototypes ---------------------------------------------*/
int IMU_Init(IMU_t *imu);
IMU_t *IMU_GetDevice(void);

int IMU_CaliGyro(IMU_t *imu);

int IMU_StartReceiving(IMU_t *imu);

int IMU_Parse(IMU_t *imu);
int IMU_Restart(void);
