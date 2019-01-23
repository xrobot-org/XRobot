#include "tool_protocol.h"

#include <string.h>

#define RC_CH_VALUE_OFFSET 0


void Protocol_DecodePC(Protocol_PcInfoDownTypeDef *hp, const uint8_t *raw) {
	
}

void Protocol_EncodePC(uint8_t *raw, const Protocol_PcInfoUpTypeDef *hp) {
	
}

void PID_DecodeJudge(Protocol_JudgeInfoTypeDef *hp, const uint8_t *raw) {
	memcpy(hp, raw, sizeof(Protocol_JudgeInfoTypeDef));
}

void PID_EncodeJudge(uint8_t *raw, const Protocol_JudgeInfoTypeDef *hp) {
	memcpy(raw, hp, sizeof(Protocol_JudgeInfoTypeDef));
}

/* Made some modification. Be aware when debug.*/
void Protocol_DecodeRemote(Protocol_RemoteInfoTypeDef *hp, const uint8_t *raw) {
	hp->rc.ch[0] = (raw[0] | (raw[1] << 8)) & 0x07ff;        
	hp->rc.ch[1] = ((raw[1] >> 3) | (raw[2] << 5)) & 0x07ff; 
	hp->rc.ch[2] = ((raw[2] >> 6) | (raw[3] << 2) | (raw[4] << 10)) & 0x07ff;
	hp->rc.ch[3] = ((raw[4] >> 1) | (raw[5] << 7)) & 0x07ff;
	/* Switch left */
	hp->rc.sw[0] = ((raw[5] >> 4) & 0x3);                  
	/* Switch right */
	hp->rc.sw[1] = ((raw[5] >> 4) & 0xC) >> 2;       
	
	hp->mouse.x = raw[6] | (raw[7] << 8);
	hp->mouse.y = raw[8] | (raw[9] << 8);
	hp->mouse.z = raw[10] | (raw[11] << 8);
	hp->mouse.press_left = raw[12];
	hp->mouse.press_right = raw[13];
	hp->key = raw[14] | (raw[15] << 8);
	hp->rc.ch[4] = raw[16] | (raw[17] << 8);

	hp->rc.ch[0] -= RC_CH_VALUE_OFFSET;
	hp->rc.ch[1] -= RC_CH_VALUE_OFFSET;
	hp->rc.ch[2] -= RC_CH_VALUE_OFFSET;
	hp->rc.ch[3] -= RC_CH_VALUE_OFFSET;
	hp->rc.ch[4] -= RC_CH_VALUE_OFFSET;
}
