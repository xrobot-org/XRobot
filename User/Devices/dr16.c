/* 
	DR16接收机

*/

/* Includes ------------------------------------------------------------------*/
#include "dr16.h"

/* Include 标准库 */
/* Include BSP相关的头文件*/
#include "bsp_uart.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static DR16_t *gdr16;
static bool inited = false;

/* Private function  ---------------------------------------------------------*/
void DR16_RxCpltCallback(void) {
	osThreadFlagsSet(gdr16->thread_alert, DR16_SIGNAL_RAW_REDY);
}

static bool DR16_DataCorrupted(const DR16_t *dr16) {
	if (dr16 == NULL)
		return DR16_ERR_NULL;
	
	const uint16_t ch_r_x = 0x07ff & (dr16->raw[0] | (dr16->raw[1] << 8));
	const uint16_t ch_r_y = 0x07ff & ((dr16->raw[1] >> 3) | (dr16->raw[2] << 5));
	const uint16_t ch_l_x = 0x07ff & ((dr16->raw[2] >> 6) | (dr16->raw[3] << 2) | (dr16->raw[4] << 10));
	const uint16_t ch_l_y = 0x07ff & ((dr16->raw[4] >> 1) | (dr16->raw[5] << 7));
	
	if ((ch_r_x < DR16_CH_VALUE_MIN) || (ch_r_x > DR16_CH_VALUE_MAX))
		return true;
	
	if ((ch_r_y < DR16_CH_VALUE_MIN) || (ch_r_y > DR16_CH_VALUE_MAX))
		return true;
	
	if ((ch_l_x < DR16_CH_VALUE_MIN) || (ch_l_x > DR16_CH_VALUE_MAX))
		return true;
	
	if ((ch_l_y < DR16_CH_VALUE_MIN) || (ch_l_y > DR16_CH_VALUE_MAX))
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
	
	BSP_UART_RegisterCallback(BSP_UART_DR16, BSP_UART_RX_COMPLETE_CB, DR16_RxCpltCallback);
	
	gdr16 = dr16;
	inited = true;
	return DR16_OK;
}

DR16_t *DR16_GetDevice(void) {
	if (inited)
		return gdr16;
	
	return NULL;
}

int8_t DR16_Restart(void) {
	// TODO
	return DR16_OK;
}

int8_t DR16_StartReceiving(DR16_t *dr16) {
	return BSP_UART_ReceiveDMA(BSP_UART_DR16, dr16->raw, DR16_RX_BUF_LENGTH);
}

int8_t DR16_Parse(const DR16_t *dr16, CMD_RC_t *rc)  {
	if (dr16 == NULL)
		return DR16_ERR_NULL;
	
	if (DR16_DataCorrupted(dr16)) {
		return DR16_ERR_NULL;
	} else {
		memset(rc, 0, sizeof(*rc));
	}
	
	const uint16_t ch_r_x = 0x07ff & (dr16->raw[0] | (dr16->raw[1] << 8));
	const uint16_t ch_r_y = 0x07ff & ((dr16->raw[1] >> 3) | (dr16->raw[2] << 5));
	const uint16_t ch_l_x = 0x07ff & ((dr16->raw[2] >> 6) | (dr16->raw[3] << 2) | (dr16->raw[4] << 10));
	const uint16_t ch_l_y = 0x07ff & ((dr16->raw[4] >> 1) | (dr16->raw[5] << 7));
	
	rc->ch_r_x = (float32_t)(ch_r_x - DR16_CH_VALUE_MID) / (float32_t)(DR16_CH_VALUE_MAX - DR16_CH_VALUE_MIN);
	rc->ch_r_y = (float32_t)(ch_r_y - DR16_CH_VALUE_MID) / (float32_t)(DR16_CH_VALUE_MAX - DR16_CH_VALUE_MIN);
	rc->ch_l_x = (float32_t)(ch_l_x - DR16_CH_VALUE_MID) / (float32_t)(DR16_CH_VALUE_MAX - DR16_CH_VALUE_MIN);
	rc->ch_l_y = (float32_t)(ch_l_y - DR16_CH_VALUE_MID) / (float32_t)(DR16_CH_VALUE_MAX - DR16_CH_VALUE_MIN);
	
	rc->sw_l = (CMD_SwitchPos_t)((dr16->raw[5] >> 4) & 0x3);
	rc->sw_r = (CMD_SwitchPos_t)(((dr16->raw[5] >> 4) & 0xC) >> 2);
	
	rc->mouse.x = dr16->raw[6] | (dr16->raw[7] << 8);
	rc->mouse.y = dr16->raw[8] | (dr16->raw[9] << 8);
	rc->mouse.z = dr16->raw[10] | (dr16->raw[11] << 8);
	
	rc->mouse.l_click = dr16->raw[12];
	rc->mouse.r_click = dr16->raw[13];
	
	rc->key = dr16->raw[14] | (dr16->raw[15] << 8);
	
	rc->ch_res = dr16->raw[16] | (dr16->raw[17] << 8);
	// TODO: TEST
	return DR16_OK;
}

