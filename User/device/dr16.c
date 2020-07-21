/* 
	DR16接收机

*/

/* Includes ------------------------------------------------------------------*/
#include "dr16.h"

#include "bsp\uart.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static DR16_t *gdr16;
static bool inited = false;

/* Private function  ---------------------------------------------------------*/
static void DR16_RxCpltCallback(void) {
	osThreadFlagsSet(gdr16->thread_alert, SIGNAL_DR16_RAW_REDY);
}

static bool DR16_DataCorrupted(const DR16_t *dr16) {
	if (dr16 == NULL)
		return DEVICE_ERR_NULL;
	
	if ((dr16->data.ch_r_x < DR16_CH_VALUE_MIN) || (dr16->data.ch_r_x > DR16_CH_VALUE_MAX))
		return true;
	
	if ((dr16->data.ch_r_y < DR16_CH_VALUE_MIN) || (dr16->data.ch_r_y > DR16_CH_VALUE_MAX))
		return true;
	
	if ((dr16->data.ch_l_x < DR16_CH_VALUE_MIN) || (dr16->data.ch_l_x > DR16_CH_VALUE_MAX))
		return true;
	
	if ((dr16->data.ch_l_y < DR16_CH_VALUE_MIN) || (dr16->data.ch_l_y > DR16_CH_VALUE_MAX))
		return true;
	
    if (dr16->data.sw_l == 0)
		return true;
	
    if (dr16->data.sw_r == 0)
		return true;
	
	return false;
}

/* Exported functions --------------------------------------------------------*/
int8_t DR16_Init(DR16_t *dr16, osThreadId_t thread_alert) {
	if (dr16 == NULL)
		return -1;
	
	if (inited)
		return -1;
	
	dr16->thread_alert = thread_alert;
	
	BSP_UART_RegisterCallback(BSP_UART_DR16, BSP_UART_RX_CPLT_CB, DR16_RxCpltCallback);
	
	gdr16 = dr16;
	inited = true;
	return DEVICE_OK;
}

DR16_t *DR16_GetDevice(void) {
	if (inited)
		return gdr16;
	
	return NULL;
}

int8_t DR16_Restart(void) {
	__HAL_UART_DISABLE(BSP_UART_GetHandle(BSP_UART_DR16));
	__HAL_UART_ENABLE(BSP_UART_GetHandle(BSP_UART_DR16));
	return DEVICE_OK;
}

int8_t DR16_StartReceiving(DR16_t *dr16) {
	if (HAL_UART_Receive_DMA(BSP_UART_GetHandle(BSP_UART_DR16), (uint8_t*)&(dr16->data), sizeof(DR16_Data_t)) == HAL_OK)
		return DEVICE_OK;
	return DEVICE_ERR;
}

int8_t DR16_ParseRC(const DR16_t *dr16, CMD_RC_t *rc)  {
	if (dr16 == NULL)
		return DEVICE_ERR_NULL;
	
	if (DR16_DataCorrupted(dr16)) {
		return DEVICE_ERR;
	} else {
		memset(rc, 0, sizeof(*rc));
	}
	
	float full_range = (float)(DR16_CH_VALUE_MAX - DR16_CH_VALUE_MIN);
	
	rc->ch_r_x = (float)(dr16->data.ch_r_x - DR16_CH_VALUE_MID) / full_range;
	rc->ch_r_y = (float)(dr16->data.ch_r_y - DR16_CH_VALUE_MID) / full_range;
	rc->ch_l_x = (float)(dr16->data.ch_l_x - DR16_CH_VALUE_MID) / full_range;
	rc->ch_l_y = (float)(dr16->data.ch_l_y - DR16_CH_VALUE_MID) / full_range;
	
	rc->sw_l = (CMD_SwitchPos_t)dr16->data.sw_l;
	rc->sw_r = (CMD_SwitchPos_t)dr16->data.sw_r;
	
	rc->mouse.x = dr16->data.x;
	rc->mouse.y = dr16->data.y;
	rc->mouse.z = dr16->data.z;
	
	rc->mouse.l_click = dr16->data.press_l;
	rc->mouse.r_click = dr16->data.press_l;
	
	rc->key = dr16->data.key;
	
	rc->ch_res = (float)(dr16->data.res - DR16_CH_VALUE_MID) / full_range;
	// TODO: TEST
	return DEVICE_OK;
}
