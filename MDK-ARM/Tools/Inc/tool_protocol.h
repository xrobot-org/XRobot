#ifndef __TOOL_PROTOCOL__H
#define __TOOL_PROTOCOL__H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	int16_t i;
	/* Gimble_ModeTypedef mode */
	/* Shoot_ModeTypedef mode */
	/* Chassis_ModeTypedef mode */
	
} Protocol_PcInfoTypeDef;

typedef struct {
	int16_t i;
} Protocol_JudgeInfoTypeDef;

void PID_DecodePC(Protocol_PcInfoTypeDef *hp, const uint8_t *raw);
void PID_EncodePC(Protocol_PcInfoTypeDef *hp, uint8_t *raw);

void PID_DecodeJudge(Protocol_JudgeInfoTypeDef *hp, const uint8_t *raw);
void PID_EncodeJudge(Protocol_JudgeInfoTypeDef *hp, uint8_t *raw);


#endif
