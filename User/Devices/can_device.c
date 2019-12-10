#include "can_device.h"

#include "main.h"
#include "can.h"


Board_Status_t CAN_Device_Init(bool use_motor, bool use_uwb) {
	CAN_FilterTypeDef  can_filter;

  can_filter.FilterMode =  CAN_FILTERMODE_IDLIST;
  can_filter.FilterScale = CAN_FILTERSCALE_32BIT;
  can_filter.FilterIdHigh = 0;
  can_filter.FilterIdLow  = 0;
  can_filter.FilterMaskIdHigh = 0;
  can_filter.FilterMaskIdLow  = 0;
  can_filter.FilterActivation = ENABLE;
  can_filter.SlaveStartFilterBank  = 14;

	if (use_motor) {
		can_filter.FilterBank = 0;
		can_filter.FilterFIFOAssignment = MOTOR_CAN_RX_FIFO; // assign to fifo0
		HAL_CAN_ConfigFilter(&hcan1, &can_filter);
		HAL_CAN_Start(&hcan1);                          // start can1
	}
	
	if (use_uwb) {
		can_filter.FilterBank = 14;
		can_filter.FilterFIFOAssignment = UWB_CAN_RX_FIFO; // assign to fifo1
		HAL_CAN_ConfigFilter(&hcan2, &can_filter);
		HAL_CAN_Start(&hcan2);                          // start can2
	}
}

Board_Status_t CAN_Motor_Init(Motor_t *mlist, uint8_t num) {
	if (mlist == NULL)
		return BOARD_FAIL;
	
	for(uint8_t i = 0; i<num; i++) {
		switch (mlist[i].model) {
			case GM6020:
				if ((can_devices.gm6020_num + 1) > GM6020_MOTOR_MAX_NUM)
					return BOARD_FAIL;
				
				can_devices.gm6020_num++;
				break;
			
			case M3508:
			case M2006:
				if ((can_devices.m3508_m2006_num + 1) > M3508_M2006_MOTOR_MAX_NUM)
					return BOARD_FAIL;
				
				can_devices.m3508_m2006_num++;
				break;
		}
	}
  HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING); // enable can1 rx interrupt
}

Board_Status_t CAN_Motor_Receive(Motor_t *mlist) {
	if (mlist == NULL)
		return BOARD_FAIL;
	
	CAN_RxHeaderTypeDef rx_header;
	uint8_t             rx_data[8];
	
	HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rx_header, rx_data); //receive can data
	
	uint8_t index;
	
  if ((rx_header.StdId >= GM6020_FEEDBACK_ID_BASE) && (rx_header.StdId <  GM6020_FEEDBACK_ID_BASE + can_devices.gm6020_num)) {
    index = rx_header.StdId - GM6020_FEEDBACK_ID_BASE; // get motor index by can id

	} else if ((rx_header.StdId >= GM6020_FEEDBACK_ID_BASE) && (rx_header.StdId <  GM6020_FEEDBACK_ID_BASE + can_devices.gm6020_num)) {
    index = rx_header.StdId - GM6020_FEEDBACK_ID_BASE; // get motor index by can id
	}
	
	mlist[index].feedback.rotor_angle    = ((rx_data[0] << 8) | rx_data[1]);
	mlist[index].feedback.rotor_speed    = ((rx_data[2] << 8) | rx_data[3]);
	mlist[index].feedback.torque_current = ((rx_data[4] << 8) | rx_data[5]);
	mlist[index].feedback.temp           =   rx_data[6];
}

Board_Status_t CAN_Motor_Control(Motor_t *m, int16_t* values) {
	CAN_TxHeaderTypeDef tx_header;
  uint8_t             tx_data[2 * can_devices.gm6020_num];

  tx_header.StdId = GM6020_CONTROL_ID_BASE;
  tx_header.IDE   = CAN_ID_STD;
  tx_header.RTR   = CAN_RTR_DATA;
  tx_header.DLC   = 8;
	
	for(int8_t i = 0; i < can_devices.gm6020_num; i++) {
		if (voltage[i] > GM6020_MOTOR_MAX_ABS_VOLTAGE)
			voltage[i]  = GM6020_MOTOR_MAX_ABS_VOLTAGE;
		
		else if (voltage[i] < GM6020_MOTOR_MAX_ABS_VOLTAGE)
			voltage[i]  = -GM6020_MOTOR_MAX_ABS_VOLTAGE;
			
		tx_data[2 * i] = (voltage[i] >> 8) & 0xff;
		tx_data[2 * i + 1] = (voltage[i]) & 0xff;
	}
	
  HAL_CAN_AddTxMessage(&hcan1, &tx_header, tx_data, (uint32_t*)CAN_TX_MAILBOX0); 
};


Board_Status_t CAN_Motor_LimitOutput(Motor_t *m, float percent) {
	
}
