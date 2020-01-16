/* 
	CAN总线上的设备的配置和运行。

*/

/* Includes ------------------------------------------------------------------*/
/* Include 自身的头文件，main.h头文件。*/
#include "can_device.h"
#include "main.h"

/* Include HAL相关的头文件。*/
#include "can.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static CAN_Motor_Feedback_t m_fb[8];
static CAN_UWB_Feedback_t u_fb;
static CAN_SuperCap_Feedback_t sc_fb;
static osThreadId_t allert_id = NULL;

/* Private function prototypes -----------------------------------------------*/
static void CAN_Motor_Decode(CAN_Motor_Feedback_t* fb, const uint8_t* raw) {
	fb->rotor_angle    = ((raw[0] << 8) | raw[1]);
	fb->rotor_speed    = ((raw[2] << 8) | raw[3]);
	fb->torque_current = ((raw[4] << 8) | raw[5]);
	fb->temp           =   raw[6];
}

static void CAN_UWB_Decode(CAN_UWB_Feedback_t* fb, const uint8_t* raw) {
	memcmp(fb->raw,raw,8);
}

static void CAN_SuperCap_Decode(CAN_SuperCap_Feedback_t* fb, const uint8_t* raw) {
}


Board_Status_t CAN_Device_Init(osThreadId_t* id) {
	if (id == NULL)
		allert_id = NULL;
	else
		allert_id = *id;
	
	CAN_FilterTypeDef  can_filter = {0};

	can_filter.FilterBank = 0;
	can_filter.FilterIdHigh = 0;
	can_filter.FilterIdLow  = 0;
	can_filter.FilterMode =  CAN_FILTERMODE_IDLIST;
	can_filter.FilterScale = CAN_FILTERSCALE_32BIT;
	can_filter.FilterMaskIdHigh = 0;
	can_filter.FilterMaskIdLow  = 0;
	can_filter.FilterActivation = ENABLE;
	can_filter.SlaveStartFilterBank  = 14;
	can_filter.FilterFIFOAssignment = MOTOR_CAN_RX_FIFO;
		
	HAL_CAN_ConfigFilter(&hcan1, &can_filter);
	HAL_CAN_Start(&hcan1);
	HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

	
	can_filter.FilterBank = 14;
	can_filter.FilterFIFOAssignment = UWB_CAN_RX_FIFO;
	HAL_CAN_ConfigFilter(&hcan2, &can_filter);
	HAL_CAN_Start(&hcan2);
	
	HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO1_MSG_PENDING);

	return BOARD_OK;
}

Board_Status_t CAN_Motor_GetFeedback(CAN_Motor_Feedback_t* pm_fb) {
	memcpy(pm_fb, m_fb, MOTOR_MAX_NUM * sizeof(CAN_Motor_Feedback_t));
	
	return BOARD_OK;
}

Board_Status_t CAN_Motor_ControlGimbal(float yaw_speed, float pitch_speed, float fric_speed, float trig_speed) {
	int16_t yaw_motor = yaw_speed * GM6020_MAX_ABS_VOLTAGE;
	int16_t pitch_motor = pitch_speed * GM6020_MAX_ABS_VOLTAGE;
	int16_t fric_motor = fric_speed * M3508_MAX_ABS_VOLTAGE;
	int16_t trig_motor = trig_speed * M2006_MAX_ABS_VOLTAGE;
	
	CAN_TxHeaderTypeDef tx_header;

	tx_header.IDE   = CAN_ID_STD;
	tx_header.RTR   = CAN_RTR_DATA;
	tx_header.DLC   = 8;
	
	tx_header.StdId = M3508_M2006_RECEIVE_ID_EXTAND;

	uint8_t tx_data[8];
	tx_data[0] = fric_motor >> 8;
    tx_data[1] = fric_motor;
    tx_data[2] = fric_motor >> 8;
    tx_data[3] = fric_motor;
    tx_data[4] = trig_motor >> 8;
    tx_data[5] = trig_motor;
    tx_data[6] = 0;
    tx_data[7] = 0;
	
	HAL_CAN_AddTxMessage(&hcan1, &tx_header, tx_data, (uint32_t*)CAN_TX_MAILBOX0); 
	
	
	tx_header.StdId = GM6020_RECEIVE_ID_EXTAND;

	tx_data[0] = yaw_motor >> 8;
    tx_data[1] = yaw_motor;
    tx_data[2] = pitch_motor >> 8;
    tx_data[3] = pitch_motor;
    tx_data[4] = 0;
    tx_data[5] = 0;
    tx_data[6] = 0;
    tx_data[7] = 0;
	
	HAL_CAN_AddTxMessage(&hcan1, &tx_header, tx_data, (uint32_t*)CAN_TX_MAILBOX0); 
	
	return BOARD_OK;
}

Board_Status_t CAN_Motor_ControlChassis(float m1_speed, float m2_speed, float m3_speed, float m4_speed) {
	CAN_TxHeaderTypeDef tx_header;

	tx_header.IDE   = CAN_ID_STD;
	tx_header.RTR   = CAN_RTR_DATA;
	tx_header.DLC   = 8;
	tx_header.StdId = M3508_M2006_RECEIVE_ID_BASE;

	
	int16_t motor1 = m1_speed * M3508_MAX_ABS_VOLTAGE;
	int16_t motor2 = m2_speed * M3508_MAX_ABS_VOLTAGE;
	int16_t motor3 = m3_speed * M3508_MAX_ABS_VOLTAGE;
	int16_t motor4 = m4_speed * M3508_MAX_ABS_VOLTAGE;
	
	uint8_t tx_data[8];
	tx_data[0] = motor1 >> 8;
    tx_data[1] = motor1;
    tx_data[2] = motor2 >> 8;
    tx_data[3] = motor2;
    tx_data[4] = motor3 >> 8;
    tx_data[5] = motor3;
    tx_data[6] = motor4 >> 8;
    tx_data[7] = motor4;
	
	HAL_CAN_AddTxMessage(&hcan1, &tx_header, tx_data, (uint32_t*)CAN_TX_MAILBOX0); 
	
	return BOARD_OK;
}


Board_Status_t CAN_Motor_3508QuickIdSetMode(void) {
	CAN_TxHeaderTypeDef tx_header;

	tx_header.IDE   = CAN_ID_STD;
	tx_header.RTR   = CAN_RTR_DATA;
	tx_header.DLC   = 8;
	tx_header.StdId = M3508_M2006_ID_SETTING_ID;
	
	uint8_t tx_data[8];

	HAL_CAN_AddTxMessage(&hcan1, &tx_header, tx_data, (uint32_t*)CAN_TX_MAILBOX0); 
}

void RxFifo0MsgPendingCallback(void) {
	CAN_RxHeaderTypeDef rx_header;
	uint8_t             rx_data[8];
	
	HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rx_header, rx_data);
	
	uint8_t index;
	
	if (rx_header.StdId >= GM6020_FEEDBACK_ID_BASE) {
		index = rx_header.StdId - GM6020_FEEDBACK_ID_BASE;

		CAN_Motor_Decode(m_fb+index, rx_data);
		if (allert_id != NULL)
			osThreadFlagsSet(allert_id, CAN_DEVICE_SIGNAL_MOTOR_RECV);
		
	} else if (rx_header.StdId == SUPERCAP_FEEDBACK_ID_BASE){
		CAN_SuperCap_Decode(&sc_fb, rx_data);
		if (allert_id != NULL)
			osThreadFlagsSet(allert_id, CAN_DEVICE_SIGNAL_SUPERCAP_RECV);
	}
}


void RxFifo1MsgPendingCallback(void) {
	CAN_RxHeaderTypeDef rx_header;
	uint8_t             rx_data[8];
	
	HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rx_header, rx_data);
	
	if (rx_header.StdId == UWB_FEEDBACK_ID_BASE)
		CAN_UWB_Decode(&u_fb, rx_data);
		if (allert_id != NULL)
			osThreadFlagsSet(allert_id, CAN_DEVICE_SIGNAL_UWB_RECV);
}
