#pragma once
/* Includes ------------------------------------------------------------------*/

/* Include board.h头文件。*/
#include "board.h"

/* Include cmsis_os2.h头文件。*/
#include "cmsis_os2.h"

/* Exported constants --------------------------------------------------------*/

/* Exported defines ----------------------------------------------------------*/
/* Motor */
#define GM6020_FEEDBACK_ID_BASE				0x205
#define GM6020_RECEIVE_ID_BASE				0x1ff
#define GM6020_RECEIVE_ID_EXTAND			0x2ff

#define M3508_M2006_FEEDBACK_ID_BASE			0x201
#define M3508_M2006_RECEIVE_ID_BASE				0x200
#define M3508_M2006_RECEIVE_ID_EXTAND			0x1ff
#define M3508_M2006_ID_SETTING_ID				0x700

#define GM6020_MAX_ABS_VOLTAGE			30000
#define M3508_MAX_ABS_VOLTAGE				16384
#define M2006_MAX_ABS_VOLTAGE				10000

#define MOTOR_MAX_NUM			9

#define MOTOR_MAX_ANGLE			8191
#define MOTOR_CAN_RX_FIFO		CAN_RX_FIFO0

/* UWB */
#define UWB_FEEDBACK_ID_BASE		(0x259)
#define UWB_RX_FIFO_SIZE				(22)
#define UWB_TX_FIFO_SIZE				22)

#define UWB_MAX_YAW					36000
#define UWB_CAN_RX_FIFO			CAN_RX_FIFO1

/* Super capacitor */
#define SUPERCAP_FEEDBACK_ID_BASE				0x000
#define SUPERCAP_RECEIVE_ID_BASE				0x000

/* Exported macro ------------------------------------------------------------*/
#define CAN_DEVICE_SIGNAL_MOTOR_RECV					(1u<<0)
#define CAN_DEVICE_SIGNAL_UWB_RECV						(1u<<1)
#define CAN_DEVICE_SIGNAL_SUPERCAP_RECV				(1u<<2)

/* Exported types ------------------------------------------------------------*/
/* Motor */
typedef enum
{
    CAN_3508_M1_ID = 0x201, /* 1 */
    CAN_3508_M2_ID = 0x202, /* 2 */
    CAN_3508_M3_ID = 0x203, /* 3 */
    CAN_3508_M4_ID = 0x204, /* 4 */
    CAN_3508_FRIC1_ID = 0x205, /* 5 */
    CAN_3508_FRIC2_ID = 0x206, /* 6 */
    CAN_2006_TRIGGER__ID = 0x207, /* 7 */

    CAN_GM6020_YAW_ID = 0x209, /* 5 */
    CAN_GM6020_PIT_ID = 0x20A, /* 6 */
} CAN_Motor_Id_t;

typedef struct {
	uint16_t rotor_angle;
	int16_t  rotor_speed;
	int16_t  torque_current;
	uint8_t  temp;
} CAN_Motor_Feedback_t;

/* UWB */
typedef union {
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
	} data;
} CAN_UWB_Feedback_t;

/* Super capacitor */
typedef struct {
	uint16_t cap_voltage;
	int16_t  battery_voltage;
	int16_t  power_limit;
	uint8_t  temp;
} CAN_SuperCap_Feedback_t;

typedef struct {
	int16_t power_limit;
} CAN_SuperCap_Control_t;


/* Exported functions prototypes ---------------------------------------------*/
/* Universal function */
Board_Status_t CAN_Device_Init(osThreadId_t* id);

/* Motor function */
Board_Status_t CAN_Motor_GetFeedback(CAN_Motor_Feedback_t* pm_fb);
Board_Status_t CAN_Motor_ControlGimbal(float yaw_speed, float pitch_speed, float fric_speed, float trig_speed);
Board_Status_t CAN_Motor_ControlChassis(float m1_speed, float m2_speed, float m3_speed, float m4_speed);

Board_Status_t CAN_Motor_3508QuickIdSetMode(void);



/* UWB function */
Board_Status_t CAN_UWB_GetFeedback(CAN_UWB_Feedback_t *puwb);

/* Super capacitor function */
Board_Status_t CAN_SuperCap_GetFeedback(CAN_Motor_Feedback_t* pm_fb);
Board_Status_t CAN_SuperCap_Control(CAN_SuperCap_Control_t* pm_ctrl);

