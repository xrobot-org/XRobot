/* 
	CAN总线上的设
	将所有CAN总线上挂载的设抽象成一设进行配和控制
*/

/* Includes ------------------------------------------------------------------*/
#include "can_device.h"

/* Include 标准库 */
#include <stdbool.h>
#include <string.h>

/* Include HAL相关的头文件 */
#include "can.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static CAN_Device_t *gcan_device;

static volatile uint8_t motor_received = 0;
static volatile uint32_t unknown_message = 0;

static CAN_RxHeaderTypeDef rx_header;
static uint8_t rx_data[8];

static bool inited = false;

/* Private function  ---------------------------------------------------------*/
static void CAN_Motor_Decode(CAN_MotorFeedback_t *fb, const uint8_t *raw) {
	fb->rotor_angle    = ((raw[0] << 8) | raw[1]);
	fb->rotor_speed    = ((raw[2] << 8) | raw[3]);
	fb->torque_current = ((raw[4] << 8) | raw[5]);
	fb->temp           =   raw[6];
	
	motor_received++;
}

static void CAN_UWB_Decode(CAN_UWBFeedback_t *fb, const uint8_t *raw) {
	memcmp(fb->raw,raw,8);
}

static void CAN_SuperCap_Decode(CAN_SuperCapFeedback_t *fb, const uint8_t *raw) {
	
}

/* Exported functions --------------------------------------------------------*/
int CAN_DeviceInit(CAN_Device_t *can_device) {
	if (can_device == NULL)
		return CAN_ERR_NULL;
	
	if (inited)
		return CAN_ERR_INITED;
	
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
	can_filter.FilterFIFOAssignment = CAN_MOTOR_CAN_RX_FIFO;
		
	HAL_CAN_ConfigFilter(&hcan1, &can_filter);
	HAL_CAN_Start(&hcan1);
	HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

	
	can_filter.FilterBank = 14;
	can_filter.FilterFIFOAssignment = CAN_UWB_CAN_RX_FIFO;
	HAL_CAN_ConfigFilter(&hcan2, &can_filter);
	HAL_CAN_Start(&hcan2);
	
	HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO1_MSG_PENDING);

	gcan_device = can_device;
	inited = true;
	return CAN_OK;
}

CAN_Device_t *CAN_GetDevice(void) {
	if (inited) {
		return gcan_device;
	}
	return NULL;
}

int CAN_Motor_ControlChassis(float m1, float m2, float m3, float m4) {
	int16_t motor1 = m1 * CAN_M3508_MAX_ABS_VOLTAGE;
	int16_t motor2 = m2 * CAN_M3508_MAX_ABS_VOLTAGE;
	int16_t motor3 = m3 * CAN_M3508_MAX_ABS_VOLTAGE;
	int16_t motor4 = m4 * CAN_M3508_MAX_ABS_VOLTAGE;
	
	CAN_TxHeaderTypeDef tx_header;

	tx_header.StdId = CAN_M3508_M2006_RECEIVE_ID_BASE;
	tx_header.IDE   = CAN_ID_STD;
	tx_header.RTR   = CAN_RTR_DATA;
	tx_header.DLC   = 8;

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
	
	return CAN_OK;
}

int CAN_Motor_ControlGimbal(float yaw, float pitch) {
	int16_t yaw_motor = yaw * CAN_GM6020_MAX_ABS_VOLTAGE;
	int16_t pitch_motor = pitch * CAN_GM6020_MAX_ABS_VOLTAGE;
	
	CAN_TxHeaderTypeDef tx_header;

	tx_header.StdId = CAN_GM6020_RECEIVE_ID_EXTAND;
	tx_header.IDE   = CAN_ID_STD;
	tx_header.RTR   = CAN_RTR_DATA;
	tx_header.DLC   = 8;

	uint8_t tx_data[8];
	tx_data[0] = yaw_motor >> 8;
    tx_data[1] = yaw_motor;
    tx_data[2] = pitch_motor >> 8;
    tx_data[3] = pitch_motor;
    tx_data[4] = 0;
    tx_data[5] = 0;
    tx_data[6] = 0;
    tx_data[7] = 0;
	
	HAL_CAN_AddTxMessage(&hcan1, &tx_header, tx_data, (uint32_t*)CAN_TX_MAILBOX0); 
	
	return CAN_OK;
}

int CAN_Motor_ControlShoot(float fric1, float fric2, float trig) {
	int16_t fric1_motor = fric1 * CAN_M3508_MAX_ABS_VOLTAGE;
	int16_t fric2_motor = fric2 * CAN_M3508_MAX_ABS_VOLTAGE;
	int16_t trig_motor = trig * CAN_M2006_MAX_ABS_VOLTAGE;
	
	CAN_TxHeaderTypeDef tx_header;

	tx_header.StdId = CAN_M3508_M2006_RECEIVE_ID_EXTAND;
	tx_header.IDE   = CAN_ID_STD;
	tx_header.RTR   = CAN_RTR_DATA;
	tx_header.DLC   = 8;
	
	uint8_t tx_data[8];
	tx_data[0] = fric1_motor >> 8;
    tx_data[1] = fric1_motor;
    tx_data[2] = fric2_motor >> 8;
    tx_data[3] = fric2_motor;
    tx_data[4] = trig_motor >> 8;
    tx_data[5] = trig_motor;
    tx_data[6] = 0;
    tx_data[7] = 0;
	
	HAL_CAN_AddTxMessage(&hcan1, &tx_header, tx_data, (uint32_t*)CAN_TX_MAILBOX0);
	
	return CAN_OK;
}

int CAN_Motor_QuickIdSetMode(void) {
	CAN_TxHeaderTypeDef tx_header;

	tx_header.StdId = CAN_M3508_M2006_ID_SETTING_ID;
	tx_header.IDE   = CAN_ID_STD;
	tx_header.RTR   = CAN_RTR_DATA;
	tx_header.DLC   = 8;
	
	uint8_t tx_data[8];

	HAL_CAN_AddTxMessage(&hcan1, &tx_header, tx_data, (uint32_t*)CAN_TX_MAILBOX0); 
	return CAN_OK;
}

void RxFifo0MsgPendingCallback(void) {
	HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rx_header, rx_data);
	
	uint8_t index;
	switch (rx_header.StdId) {
		case CAN_M3508_M1_ID:
		case CAN_M3508_M2_ID:
		case CAN_M3508_M3_ID:
		case CAN_M3508_M4_ID:
			index = rx_header.StdId - CAN_M3508_M1_ID;
			CAN_Motor_Decode(&(gcan_device->chassis_motor_fb[index]), rx_data);
			break;
		
		case CAN_M3508_FRIC1_ID:
		case CAN_M3508_FRIC2_ID:
			index = rx_header.StdId - CAN_M3508_FRIC1_ID;
			CAN_Motor_Decode(&(gcan_device->gimbal_motor_fb.fric_fb[index]), rx_data);
			break;
		
		case CAN_M2006_TRIG_ID:
			CAN_Motor_Decode(&(gcan_device->gimbal_motor_fb.trig_fb), rx_data);
			break;
		
		case CAN_GM6020_YAW_ID:
			CAN_Motor_Decode(&(gcan_device->gimbal_motor_fb.yaw_fb), rx_data);
			break;
		
		case CAN_GM6020_PIT_ID:
			CAN_Motor_Decode(&(gcan_device->gimbal_motor_fb.pit_fb), rx_data);
			break;

		case CAN_SUPERCAP_FEEDBACK_ID_BASE:
			CAN_SuperCap_Decode(&(gcan_device->supercap_feedback), rx_data);
			osSignalSet(gcan_device->supercap_alert, CAN_DEVICE_SIGNAL_SUPERCAP_RECV);
			break;
		
		default:
			unknown_message++;
			break;
	}
	
	if (motor_received > CAN_CHASSIS_MOTOR_NUM) {
		for(uint8_t i = 0; i < 3; i++) {
			if(gcan_device->motor_alert[i]) {
				osSignalSet(gcan_device->motor_alert, CAN_DEVICE_SIGNAL_MOTOR_RECV);
			}
		}
		motor_received = 0;
	}
}

void RxFifo1MsgPendingCallback(void) {
	HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO1, &rx_header, rx_data);
	
	switch (rx_header.StdId) {
		case CAN_UWB_FEEDBACK_ID_BASE:
			CAN_UWB_Decode(&(gcan_device->uwb_feedback), rx_data);
			osSignalSet(gcan_device->uwb_alert, CAN_DEVICE_SIGNAL_UWB_RECV);
			break;
		
		default:
			unknown_message++;
			break;
	}
}
