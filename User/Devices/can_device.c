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
/* Private function prototypes -----------------------------------------------*/
Board_Status_t CAN_Device_Init() {
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

Board_Status_t CAN_Motor_Receive(Motor_Feedback_t* pm_fb) {
	if (pm_fb == NULL)
		return BOARD_FAIL;
	
	CAN_RxHeaderTypeDef rx_header;
	uint8_t             rx_data[8];
	
	HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rx_header, rx_data);
	
	uint8_t index;
	Motor_Model_t model;
	
	if (rx_header.StdId >= GM6020_FEEDBACK_ID_BASE) {
		index = rx_header.StdId - GM6020_FEEDBACK_ID_BASE;
		model = GM6020;

	} else if (rx_header.StdId >= M3508_M2006_FEEDBACK_ID_BASE) {
		index = rx_header.StdId - M3508_M2006_FEEDBACK_ID_BASE;
		model = M3508_M2006;
		
	} else
		return BOARD_FAIL;
	
	if (model  == pm_fb[index].model) {
		pm_fb[index].rotor_angle    = ((rx_data[0] << 8) | rx_data[1]);
		pm_fb[index].rotor_speed    = ((rx_data[2] << 8) | rx_data[3]);
		pm_fb[index].torque_current = ((rx_data[4] << 8) | rx_data[5]);
		pm_fb[index].temp           =   rx_data[6];
	} else
		return BOARD_FAIL;
	
	return BOARD_OK;
}

Board_Status_t CAN_Motor_Control(Motor_Control_t* pm) {
	CAN_TxHeaderTypeDef tx_header;

	tx_header.IDE   = CAN_ID_STD;
	tx_header.RTR   = CAN_RTR_DATA;
	tx_header.DLC   = 8;

	int16_t max_output_current;
	uint32_t extand_id;
	
	switch(pm->model) {
		case GM6020:
			tx_header.StdId = GM6020_RECEIVE_ID_BASE;
			extand_id = GM6020_RECEIVE_ID_EXTAND;
			max_output_current = GM6020_MAX_ABS_VOLTAGE;
			break;
		
		case M3508_M2006:
			tx_header.StdId = M3508_M2006_RECEIVE_ID_BASE;
			extand_id = M3508_M2006_RECEIVE_ID_EXTAND;
			max_output_current = M3508_MOTOR_MAX_ABS_VOLTAGE;
			break;
	}
	
	uint8_t tx_data[8];
	
	for(int8_t i = 0; i < 4; i++) {
		if (pm->set_current[i] > 1.f)
			pm->set_current[i]  = 1.f;
		
		else if (pm->set_current[i] < 1.f)
			pm->set_current[i]  = -1.f;
		
		int16_t output_current = max_output_current * pm->set_current[i];
			
		tx_data[2 * i] = (output_current >> 8) & 0xff;
		tx_data[2 * i + 1] = (output_current) & 0xff;
	}
	
	HAL_CAN_AddTxMessage(&hcan1, &tx_header, tx_data, (uint32_t*)CAN_TX_MAILBOX0); 
	
	tx_header.StdId = extand_id;
	
	for(int8_t i = 4; i < 8; i++) {
		if (pm->set_current[i] > 1.f)
			pm->set_current[i]  = 1.f;
		
		else if (pm->set_current[i] < 1.f)
			pm->set_current[i]  = -1.f;
		
		int16_t output_current = max_output_current * pm->set_current[i];
			
		tx_data[2 * i] = (output_current >> 8) & 0xff;
		tx_data[2 * i + 1] = (output_current) & 0xff;
	}
	
	HAL_CAN_AddTxMessage(&hcan1, &tx_header, tx_data, (uint32_t*)CAN_TX_MAILBOX0); 
	
	return BOARD_OK;
};
