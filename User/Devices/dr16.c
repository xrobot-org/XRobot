#include "dr16.h"

#include <string.h>

#define RC_CH_VALUE_OFFSET 0


/* Made some modification. Be aware when debug.*/
void DR16_Decode(DR16_t* hp) {
	hp->rc.ch[0] = (hp->raw[0] | (hp->raw[1] << 8)) & 0x07ff;        
	hp->rc.ch[1] = ((hp->raw[1] >> 3) | (hp->raw[2] << 5)) & 0x07ff; 
	hp->rc.ch[2] = ((hp->raw[2] >> 6) | (hp->raw[3] << 2) | (hp->raw[4] << 10)) & 0x07ff;
	hp->rc.ch[3] = ((hp->raw[4] >> 1) | (hp->raw[5] << 7)) & 0x07ff;
	/* Left switch  */
	hp->rc.sw[0] = ((hp->raw[5] >> 4) & 0x3);                  
	/* Right switch  */
	hp->rc.sw[1] = ((hp->raw[5] >> 4) & 0xC) >> 2;       
	
	hp->mouse.x = hp->raw[6] | (hp->raw[7] << 8);
	hp->mouse.y = hp->raw[8] | (hp->raw[9] << 8);
	hp->mouse.z = hp->raw[10] | (hp->raw[11] << 8);
	hp->mouse.press_left = hp->raw[12];
	hp->mouse.press_right = hp->raw[13];
	hp->key = hp->raw[14] | (hp->raw[15] << 8);
	hp->rc.ch[4] = hp->raw[16] | (hp->raw[17] << 8);

	hp->rc.ch[0] -= RC_CH_VALUE_OFFSET;
	hp->rc.ch[1] -= RC_CH_VALUE_OFFSET;
	hp->rc.ch[2] -= RC_CH_VALUE_OFFSET;
	hp->rc.ch[3] -= RC_CH_VALUE_OFFSET;
	hp->rc.ch[4] -= RC_CH_VALUE_OFFSET;
}
