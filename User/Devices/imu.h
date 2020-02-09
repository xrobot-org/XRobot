#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Include cmsis_os.h头文件 */
#include "cmsis_os.h"

/* Exported constants --------------------------------------------------------*/
#define IMU_OK			0
#define IMU_ERR			-1
#define IMU_ERR_NULL	-2
#define IMU_ERR_INITED	-3
#define IMU_ERR_NO_DEV	-4

#define IMU_SIGNAL_RAW_ACCL_REDY	(1u<<0)
#define IMU_SIGNAL_RAW_GYRO_REDY	(1u<<1)
#define IMU_SIGNAL_RAW_MAGN_REDY	(1u<<2)

#define IMU_SIGNAL_DATA_REDY		(1u<<3)

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct {
	osThreadId received_alert;

	union {
		uint8_t bytes[20];
			
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
		} packed;
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
int IMU_Parse(IMU_t *imu);
int IMU_Restart(void);
