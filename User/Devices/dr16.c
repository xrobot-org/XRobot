/* 
	DR16接收机。

*/

/* Includes ------------------------------------------------------------------*/
/* Include 自身的头文件。*/
#include "dr16.h"

/* Include 标准库。*/
/* Include HAL相关的头文件。*/
#include "usart.h"
#include "dma.h"

/* Include Component相关的头文件。*/
/* Private define ------------------------------------------------------------*/
#define DR16_UART_HANDLE huart1
#define DR16_DMA_HANDLE hdma_usart1_rx
#define SBUS_RX_BUF_NUM 36u
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static DR16_t* dr16;
static bool inited = false;
static uint8_t raw[SBUS_RX_BUF_NUM];

/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
int DR16_Init(DR16_t* pdr) {
	if (pdr == NULL)
		return -1;
	if (inited)
		return -1;
	
	inited = true;
	
	dr16 = pdr;
	HAL_UART_Receive_DMA(&DR16_UART_HANDLE, raw, SBUS_RX_BUF_NUM);
	return 0;
}

/* Made some modification. Be aware when debug.*/
int DR16_Parse(DR16_t* pdr, const uint8_t* raw){
	if (pdr == NULL || raw == NULL)
		return -1;
		
	pdr->rc.ch[0] = (raw[0] | (raw[1] << 8)) & 0x07ff;        
	pdr->rc.ch[1] = ((raw[1] >> 3) | (raw[2] << 5)) & 0x07ff; 
	pdr->rc.ch[2] = ((raw[2] >> 6) | (raw[3] << 2) | (raw[4] << 10)) & 0x07ff;
	pdr->rc.ch[3] = ((raw[4] >> 1) | (raw[5] << 7)) & 0x07ff;
	/* Left switch  */
	pdr->rc.sw[0] = ((raw[5] >> 4) & 0x3);                  
	/* Right switch  */
	pdr->rc.sw[1] = ((raw[5] >> 4) & 0xC) >> 2;       
	
	pdr->mouse.x = raw[6] | (raw[7] << 8);
	pdr->mouse.y = raw[8] | (raw[9] << 8);
	pdr->mouse.z = raw[10] | (raw[11] << 8);
	pdr->mouse.press_left = raw[12];
	pdr->mouse.press_right = raw[13];
	pdr->key = raw[14] | (raw[15] << 8);
	pdr->rc.ch[4] = raw[16] | (raw[17] << 8);

	pdr->rc.ch[0] -= RC_CH_VALUE_OFFSET;
	pdr->rc.ch[1] -= RC_CH_VALUE_OFFSET;
	pdr->rc.ch[2] -= RC_CH_VALUE_OFFSET;
	pdr->rc.ch[3] -= RC_CH_VALUE_OFFSET;
	pdr->rc.ch[4] -= RC_CH_VALUE_OFFSET;
	
	return 0;
}

void HAL_UART1_RxCpltCallback()
{
	HAL_UART_DMAPause(&DR16_UART_HANDLE);
	DR16_Parse(dr16, raw);
	HAL_UART_DMAResume(&DR16_UART_HANDLE);
	osThreadFlagsSet(dr16->received_alert, DR16_SIGNAL_DATA_RECV);
}

