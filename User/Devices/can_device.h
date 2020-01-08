#pragma once

#include "board.h"

/* Motor */
#define GM6020_FEEDBACK_ID_BASE				0x205
#define GM6020_RECEIVE_ID_BASE				0x1ff
#define GM6020_RECEIVE_ID_EXTAND			0x2ff

#define M3508_M2006_FEEDBACK_ID_BASE			0x201
#define M3508_M2006_RECEIVE_ID_BASE				0x200
#define M3508_M2006_RECEIVE_ID_EXTAND			0x1ff

#define GM6020_MAX_ABS_VOLTAGE	30000
#define M3508_MOTOR_MAX_ABS_VOLTAGE	16384
#define M2006_MAX_ABS_VOLTAGE	10000

#define M3508_M2006_MAX_NUM		8
#define GM6020_MAX_NUM					7

#define MOTOR_MAX_ANGLE			8191
#define MOTOR_CAN_RX_FIFO		CAN_RX_FIFO0

/* UWB */
#define UWB_FEEDBACK_ID_BASE	(0x259)
#define UWB_RX_FIFO_SIZE			(22)
#define UWB_TX_FIFO_SIZE			(22)

#define UWB_MAX_YAW				36000
#define UWB_CAN_RX_FIFO		CAN_RX_FIFO1

/* Super capacitor */
#define SUPERCAP_FEEDBACK_ID_BASE				0x000
#define SUPERCAP_RECEIVE_ID_BASE				0x000


/* Motor */
typedef enum {
	GM6020,
	M3508_M2006,
} Motor_Model_t;

typedef struct {
	Motor_Model_t model;

	uint16_t rotor_angle;
	int16_t  rotor_speed;
	int16_t  torque_current;
	uint8_t  temp;
}	Motor_Feedback_t;

typedef struct {
	Motor_Model_t model;
	float set_current[M3508_M2006_MAX_NUM];
} Motor_Control_t;

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
		
	}data;
}UWB_t;

/* Super capacitor */
typedef struct {
	uint16_t cap_voltage;
	int16_t  battery_voltage;
	int16_t  max_power;
	uint8_t  temp;
}	SuperCap_Feedback_t;

typedef struct {
	Motor_Model_t model;
	float set_current[M3508_M2006_MAX_NUM];
} SuperCap_Control_t;

/* Universal function */
Board_Status_t CAN_Device_Init(void);

/* Motor function */
Board_Status_t CAN_Motor_Receive(Motor_Feedback_t* pm_fb);
Board_Status_t CAN_Motor_Control(Motor_Control_t* pm_ctrl);

/* UWB function */
Board_Status_t CAN_UWB_Receive(UWB_t *puwb);

/* Super capacitor function */
Board_Status_t CAN_SuperCap_Receive(Motor_Feedback_t* pm_fb);
Board_Status_t CAN_SuperCap_Control(Motor_Control_t* pm_ctrl);

