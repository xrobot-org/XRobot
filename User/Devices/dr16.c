#include "dr16.h"

#include "main.h"
#include "usart.h"


/* Made some modification. Be aware when debug.*/
Board_Status_t DR16_Decode(DR16_t* pdr, const uint8_t* raw){
	if (pdr == NULL || raw == NULL)
		return BOARD_FAIL;
		
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
	
	return BOARD_OK;
}
