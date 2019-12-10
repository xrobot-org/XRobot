#pragma once

#include "board.h"

/* Motor */
#define GM6020_FEEDBACK_ID_BASE				0x205
#define GM6020_RECEIVE_ID_BASE				0x1ff
#define GM6020_RECEIVE_ID_EXTAND			0x2ff
#define GM6020_MOTOR_MAX_ABS_VOLTAGE	30000
#define GM6020_MOTOR_MAX_NUM					7

#define M3508_FEEDBACK_ID_BASE			0x201
#define M3508_RECEIVE_ID_BASE				0x200
#define M3508_RECEIVE_ID_EXTAND			0x1ff
#define M3508_MOTOR_MAX_ABS_VOLTAGE	16384

#define M2006_FEEDBACK_ID_BASE			0x201
#define M2006_RECEIVE_ID_BASE				0x200
#define M2006_RECEIVE_ID_EXTAND			0x1ff
#define M2006_MOTOR_MAX_ABS_VOLTAGE	10000

#define M3508_M2006_MOTOR_MAX_NUM		8

#define MOTOR_MAX_ANGLE			8191
#define MOTOR_CAN_RX_FIFO		CAN_RX_FIFO0

/* UWB */
#define UWB_FEEDBACK_ID_BASE	(0x259)
#define UWB_RX_FIFO_SIZE			(22)
#define UWB_TX_FIFO_SIZE			(22)

#define UWB_MAX_YAW				36000
#define UWB_CAN_RX_FIFO		CAN_RX_FIFO1

/* Motor */

typedef enum {
	GM6020,
	M3508,
	M2006,
} Motor_Model_t;

typedef struct {
	Motor_Model_t model;
	uint16_t can_id;
	int16_t  current_setpoint;
	uint32_t power;
	
	struct {
		uint16_t rotor_angle;
		int16_t  rotor_speed;
		int16_t  torque_current;
		uint8_t  temp;
	} feedback;
}	Motor_t;

struct {
	uint8_t gm6020_num;
	uint8_t m3508_m2006_num;
} can_devices;

/* UWB */

typedef struct {
	union {
		uint8_t raw[UWB_RX_FIFO_SIZE];
		
		__packed struct
		{
			int16_t coor_x;
			int16_t corr_y;
			uint16_t yaw;
			int16_t distance[6];
			uint16_t err_mask : 14;
			uint16_t sig_level : 2;
			uint16_t reserved;
		} uwb_info_t;
		
	}info;
}UWB_t;

/* Motor function */
Board_Status_t CAN_Motor_Init(Motor_t *m, uint8_t num);
Board_Status_t CAN_Motor_Receive(Motor_t *plist);
Board_Status_t CAN_Motor_Control(Motor_t *m, int16_t* values);
Board_Status_t CAN_Motor_LimitOutput(Motor_t *m, float percent);

/* UWB function */
Board_Status_t CAN_UWB_Init(void);
Board_Status_t CAN_UWB_Receive(Motor_t *plist);
Board_Status_t CAN__Control(Motor_t *m, int16_t* values);
Board_Status_t CAN_Motor_LimitOutput(Motor_t *m, float percent);

