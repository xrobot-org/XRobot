/* 
	DR16接收机

*/

/* Includes ------------------------------------------------------------------*/
#include "dr16.h"

/* Include 标准库*/
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
	osSignalSet(gdr16->received_alert, DR16_SIGNAL_RAW_REDY);
}

/* Exported functions --------------------------------------------------------*/
int DR16_Init(DR16_t *dr16) {
	if (dr16 == NULL)
		return -1;

	if (inited)
		return -1;

	gdr16 = dr16;

	// BSP_UART_GegesterCallback(DR16_RxCpltCallback);
	inited = true;
	return 0;
}

DR16_t *DR16_GetDevice(void) {
	if (inited) {
		return gdr16;
	}
	return NULL;
}

int DR16_StartReceiving(DR16_t *dr16) {
	return BSP_UART_Receive(BSP_UART_DR16, dr16->raw, DR16_RX_BUF_NUM);
}

/* Made some modification. Be aware when debug.*/
int DR16_Parse(DR16_t *dr16) {
	if (dr16 == NULL)
		return -1;
		
	dr16->data.rc.ch[0] = (dr16->raw[0] | (dr16->raw[1] << 8)) & 0x07ff;        
	dr16->data.rc.ch[1] = ((dr16->raw[1] >> 3) | (dr16->raw[2] << 5)) & 0x07ff; 
	dr16->data.rc.ch[2] = ((dr16->raw[2] >> 6) | (dr16->raw[3] << 2) | (dr16->raw[4] << 10)) & 0x07ff;
	dr16->data.rc.ch[3] = ((dr16->raw[4] >> 1) | (dr16->raw[5] << 7)) & 0x07ff;
	/* Left switch  */
	dr16->data.rc.sw[0] = ((dr16->raw[5] >> 4) & 0x3);                  
	/* Right switch  */
	dr16->data.rc.sw[1] = ((dr16->raw[5] >> 4) & 0xC) >> 2;       
	
	dr16->data.mouse.x = dr16->raw[6] | (dr16->raw[7] << 8);
	dr16->data.mouse.y = dr16->raw[8] | (dr16->raw[9] << 8);
	dr16->data.mouse.z = dr16->raw[10] | (dr16->raw[11] << 8);
	dr16->data.mouse.press_left = dr16->raw[12];
	dr16->data.mouse.press_right = dr16->raw[13];
	dr16->data.key = dr16->raw[14] | (dr16->raw[15] << 8);
	dr16->data.rc.ch[4] = dr16->raw[16] | (dr16->raw[17] << 8);
	
	return 0;
}


