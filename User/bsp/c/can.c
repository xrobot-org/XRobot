/* Includes ------------------------------------------------------------------*/
#include "bsp\can.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
struct {
	struct {
		void (*TxMailbox0CompleteCallback)(void);
		void (*TxMailbox1CompleteCallback)(void);
		void (*TxMailbox2CompleteCallback)(void);
		void (*TxMailbox0AbortCallback)(void);
		void (*TxMailbox1AbortCallback)(void);
		void (*TxMailbox2AbortCallback)(void);
		void (*RxFifo0MsgPendingCallback)(void);
		void (*RxFifo0FullCallback)(void);
		void (*RxFifo1MsgPendingCallback)(void);
		void (*RxFifo1FullCallback)(void);
		void (*SleepCallback)(void);
		void (*WakeUpFromRxMsgCallback)(void);
		void (*ErrorCallback)(void);
	} can1;

	struct {
		void (*TxMailbox0CompleteCallback)(void);
		void (*TxMailbox1CompleteCallback)(void);
		void (*TxMailbox2CompleteCallback)(void);
		void (*TxMailbox0AbortCallback)(void);
		void (*TxMailbox1AbortCallback)(void);
		void (*TxMailbox2AbortCallback)(void);
		void (*RxFifo0MsgPendingCallback)(void);
		void (*RxFifo0FullCallback)(void);
		void (*RxFifo1MsgPendingCallback)(void);
		void (*RxFifo1FullCallback)(void);
		void (*SleepCallback)(void);
		void (*WakeUpFromRxMsgCallback)(void);
		void (*ErrorCallback)(void);
	} can2;
} static bsp_can_callback;

/* Private function  ---------------------------------------------------------*/
static CAN_TypeDef *CAN_GetInstance(BSP_CAN_t can) {
	switch (can) {
		case BSP_CAN_2:
			return CAN2;
		case BSP_CAN_1:
			return CAN1;
	}
}

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan) {
	if (hcan->Instance == CAN_GetInstance(BSP_CAN_2)) {
		if (bsp_can_callback.can2.TxMailbox0CompleteCallback)
			bsp_can_callback.can2.TxMailbox0CompleteCallback();
	} else if (hcan->Instance == CAN_GetInstance(BSP_CAN_1)) {
		if (bsp_can_callback.can1.TxMailbox0CompleteCallback)
			bsp_can_callback.can1.TxMailbox0CompleteCallback();
	}
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan) {
	if (hcan->Instance == CAN_GetInstance(BSP_CAN_2)) {
		if (bsp_can_callback.can2.TxMailbox1CompleteCallback)
			bsp_can_callback.can2.TxMailbox1CompleteCallback();
	} else if (hcan->Instance == CAN_GetInstance(BSP_CAN_1)) {
		if (bsp_can_callback.can1.TxMailbox1CompleteCallback)
			bsp_can_callback.can1.TxMailbox1CompleteCallback();
	}
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan) {
	if (hcan->Instance == CAN_GetInstance(BSP_CAN_2)) {
		if (bsp_can_callback.can2.TxMailbox2CompleteCallback)
			bsp_can_callback.can2.TxMailbox2CompleteCallback();
	} else if (hcan->Instance == CAN_GetInstance(BSP_CAN_1)) {
		if (bsp_can_callback.can1.TxMailbox2CompleteCallback)
			bsp_can_callback.can1.TxMailbox2CompleteCallback();
	}
}

void HAL_CAN_TxMailbox0AbortCallback(CAN_HandleTypeDef *hcan) {
	if (hcan->Instance == CAN_GetInstance(BSP_CAN_2)) {
		if (bsp_can_callback.can2.TxMailbox0AbortCallback)
			bsp_can_callback.can2.TxMailbox0AbortCallback();
	} else if (hcan->Instance == CAN_GetInstance(BSP_CAN_1)) {
		if (bsp_can_callback.can1.TxMailbox0AbortCallback)
			bsp_can_callback.can1.TxMailbox0AbortCallback();
	}
}

void HAL_CAN_TxMailbox1AbortCallback(CAN_HandleTypeDef *hcan) {
	if (hcan->Instance == CAN_GetInstance(BSP_CAN_2)) {
		if (bsp_can_callback.can2.TxMailbox1AbortCallback)
			bsp_can_callback.can2.TxMailbox1AbortCallback();
	} else if (hcan->Instance == CAN_GetInstance(BSP_CAN_1)) {
		if (bsp_can_callback.can1.TxMailbox1AbortCallback)
			bsp_can_callback.can1.TxMailbox1AbortCallback();
	}
}

void HAL_CAN_TxMailbox2AbortCallback(CAN_HandleTypeDef *hcan) {
	if (hcan->Instance == CAN_GetInstance(BSP_CAN_2)) {
		if (bsp_can_callback.can2.TxMailbox2AbortCallback)
			bsp_can_callback.can2.TxMailbox2AbortCallback();
	} else if (hcan->Instance == CAN_GetInstance(BSP_CAN_1)) {
		if (bsp_can_callback.can1.TxMailbox2AbortCallback)
			bsp_can_callback.can1.TxMailbox2AbortCallback();
	}
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
	if (hcan->Instance == CAN_GetInstance(BSP_CAN_2)) {
		if (bsp_can_callback.can2.RxFifo0MsgPendingCallback)
			bsp_can_callback.can2.RxFifo0MsgPendingCallback();
	} else if (hcan->Instance == CAN_GetInstance(BSP_CAN_1)) {
		if (bsp_can_callback.can1.RxFifo0MsgPendingCallback)
			bsp_can_callback.can1.RxFifo0MsgPendingCallback();
	}
}

void HAL_CAN_RxFifo0FullCallback(CAN_HandleTypeDef *hcan) {
	if (hcan->Instance == CAN_GetInstance(BSP_CAN_2)) {
		if (bsp_can_callback.can2.RxFifo0FullCallback)
			bsp_can_callback.can2.RxFifo0FullCallback();
	} else if (hcan->Instance == CAN_GetInstance(BSP_CAN_1)) {
		if (bsp_can_callback.can1.RxFifo0FullCallback)
			bsp_can_callback.can1.RxFifo0FullCallback();
	}
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) {
	if (hcan->Instance == CAN_GetInstance(BSP_CAN_2)) {
		if (bsp_can_callback.can2.RxFifo1MsgPendingCallback)
			bsp_can_callback.can2.RxFifo1MsgPendingCallback();
	} else if (hcan->Instance == CAN_GetInstance(BSP_CAN_1)) {
		if (bsp_can_callback.can1.RxFifo1MsgPendingCallback)
			bsp_can_callback.can1.RxFifo1MsgPendingCallback();
	}
}

void HAL_CAN_RxFifo1FullCallback(CAN_HandleTypeDef *hcan) {
	if (hcan->Instance == CAN_GetInstance(BSP_CAN_2)) {
		if (bsp_can_callback.can2.RxFifo1FullCallback)
			bsp_can_callback.can2.RxFifo1FullCallback();
	} else if (hcan->Instance == CAN_GetInstance(BSP_CAN_1)) {
		if (bsp_can_callback.can1.RxFifo1FullCallback)
			bsp_can_callback.can1.RxFifo1FullCallback();
	}
}

void HAL_CAN_SleepCallback(CAN_HandleTypeDef *hcan) {
	if (hcan->Instance == CAN_GetInstance(BSP_CAN_2)) {
		if (bsp_can_callback.can2.SleepCallback)
			bsp_can_callback.can2.SleepCallback();
	} else if (hcan->Instance == CAN_GetInstance(BSP_CAN_1)) {
		if (bsp_can_callback.can1.SleepCallback)
			bsp_can_callback.can1.SleepCallback();
	}
}

void HAL_CAN_WakeUpFromRxMsgCallback(CAN_HandleTypeDef *hcan) {
	if (hcan->Instance == CAN_GetInstance(BSP_CAN_2)) {
		if (bsp_can_callback.can2.WakeUpFromRxMsgCallback)
			bsp_can_callback.can2.WakeUpFromRxMsgCallback();
	} else if (hcan->Instance == CAN_GetInstance(BSP_CAN_1)) {
		if (bsp_can_callback.can1.WakeUpFromRxMsgCallback)
			bsp_can_callback.can1.WakeUpFromRxMsgCallback();
	}
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) {
	if (hcan->Instance == CAN_GetInstance(BSP_CAN_2)) {
		if (bsp_can_callback.can2.ErrorCallback)
			bsp_can_callback.can2.ErrorCallback();
	} else if (hcan->Instance == CAN_GetInstance(BSP_CAN_1)) {
		if (bsp_can_callback.can1.ErrorCallback)
			bsp_can_callback.can1.ErrorCallback();
	}
}


/* Exported functions --------------------------------------------------------*/
CAN_HandleTypeDef *BSP_CAN_GetHandle(BSP_CAN_t can) {
		switch (can) {
		case BSP_CAN_2:
			return &hcan2;
		case BSP_CAN_1:
			return &hcan1;
		/*
		case BSP_CAN_XXX:
			return &hcanX;
		*/
	}
}

int8_t BSP_CAN_RegisterCallback(BSP_CAN_t can, BSP_CAN_Callback_t type, void (*callback)(void)) {
	if (callback == NULL)
		return -1;
	
	switch (can) {
		case BSP_CAN_1:
			switch (type) {
				case HAL_CAN_TX_MAILBOX0_CPLT_CB:
					bsp_can_callback.can1.TxMailbox0CompleteCallback = callback;
					break;
				case HAL_CAN_TX_MAILBOX1_CPLT_CB:
					bsp_can_callback.can1.TxMailbox1CompleteCallback = callback;
					break;
				case HAL_CAN_TX_MAILBOX2_CPLT_CB:
					bsp_can_callback.can1.TxMailbox2CompleteCallback = callback;
					break;
				case HAL_CAN_TX_MAILBOX0_ABORT_CB:
					bsp_can_callback.can1.TxMailbox0AbortCallback = callback;
					break;
				case HAL_CAN_TX_MAILBOX1_ABORT_CB:
					bsp_can_callback.can1.TxMailbox1AbortCallback = callback;
					break;
				case HAL_CAN_TX_MAILBOX2_ABORT_CB:
					bsp_can_callback.can1.TxMailbox2AbortCallback = callback;
					break;
				case HAL_CAN_RX_FIFO0_MSG_PENDING_CB:
					bsp_can_callback.can1.RxFifo0MsgPendingCallback = callback;
					break;
				case HAL_CAN_RX_FIFO0_FULL_CB:
					bsp_can_callback.can1.RxFifo0FullCallback = callback;
					break;
				case HAL_CAN_RX_FIFO1_MSG_PENDING_CB:
					bsp_can_callback.can1.RxFifo1MsgPendingCallback = callback;
					break;
				case HAL_CAN_RX_FIFO1_FULL_CB:
					bsp_can_callback.can1.RxFifo1FullCallback = callback;
					break;
				case HAL_CAN_SLEEP_CB:
					bsp_can_callback.can1.SleepCallback = callback;
					break;
				case HAL_CAN_WAKEUP_FROM_RX_MSG_CB:
					bsp_can_callback.can1.WakeUpFromRxMsgCallback = callback;
					break;
				case HAL_CAN_ERROR_CB:
					bsp_can_callback.can1.ErrorCallback = callback;
					break;
			}
			break;

		case BSP_CAN_2:
			switch (type) {
				case HAL_CAN_TX_MAILBOX0_CPLT_CB:
					bsp_can_callback.can2.TxMailbox0CompleteCallback = callback;
					break;
				case HAL_CAN_TX_MAILBOX1_CPLT_CB:
					bsp_can_callback.can2.TxMailbox1CompleteCallback = callback;
					break;
				case HAL_CAN_TX_MAILBOX2_CPLT_CB:
					bsp_can_callback.can2.TxMailbox2CompleteCallback = callback;
					break;
				case HAL_CAN_TX_MAILBOX0_ABORT_CB:
					bsp_can_callback.can2.TxMailbox0AbortCallback = callback;
					break;
				case HAL_CAN_TX_MAILBOX1_ABORT_CB:
					bsp_can_callback.can2.TxMailbox1AbortCallback = callback;
					break;
				case HAL_CAN_TX_MAILBOX2_ABORT_CB:
					bsp_can_callback.can2.TxMailbox2AbortCallback = callback;
					break;
				case HAL_CAN_RX_FIFO0_MSG_PENDING_CB:
					bsp_can_callback.can2.RxFifo0MsgPendingCallback = callback;
					break;
				case HAL_CAN_RX_FIFO0_FULL_CB:
					bsp_can_callback.can2.RxFifo0FullCallback = callback;
					break;
				case HAL_CAN_RX_FIFO1_MSG_PENDING_CB:
					bsp_can_callback.can2.RxFifo1MsgPendingCallback = callback;
					break;
				case HAL_CAN_RX_FIFO1_FULL_CB:
					bsp_can_callback.can2.RxFifo1FullCallback = callback;
					break;
				case HAL_CAN_SLEEP_CB:
					bsp_can_callback.can2.SleepCallback = callback;
					break;
				case HAL_CAN_WAKEUP_FROM_RX_MSG_CB:
					bsp_can_callback.can2.WakeUpFromRxMsgCallback = callback;
					break;
				case HAL_CAN_ERROR_CB:
					bsp_can_callback.can2.ErrorCallback = callback;
					break;
			}
			break;
	}
	return 0;
}
