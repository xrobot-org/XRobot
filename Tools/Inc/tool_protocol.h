#ifndef __TOOL_PROTOCOL__H
#define __TOOL_PROTOCOL__H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	GIMBAL_MODE_AIM,
	GIMBAL_MODE_PATROL,
} Gimbal_ModeTypeDef;

typedef enum {
	SHOOT_MODE_IDLE,
	SHOOT_MODE_SLOW,
	SHOOT_MODE_FAST,
	SHOOT_MODE_SUICIDE,
} Shoot_ModeTypeDef;

typedef enum {
	CHASSIS_MODE_PATROL,
	CHASSIS_MODE_DODGE,
} Chassis_ModeTypeDef;

typedef __packed struct {
	Gimbal_ModeTypeDef gimbal_mode;
	Shoot_ModeTypeDef shoot_mode;
	Chassis_ModeTypeDef chassis_mode;
} Protocol_PcInfoDownTypeDef;

typedef __packed struct {
	Gimbal_ModeTypeDef gimbal_mode;
	Shoot_ModeTypeDef shoot_mode;
	Chassis_ModeTypeDef chassis_mode;
} Protocol_PcInfoUpTypeDef;

typedef __packed struct {
	int16_t i;
} Protocol_JudgeInfoTypeDef;

typedef __packed struct {
	__packed struct {
		int16_t ch[5];
		uint8_t sw[2];
	} rc;
	__packed struct {
		int16_t x;
		int16_t y;
		int16_t z;
		uint8_t press_left;
		uint8_t press_right;
	} mouse;
	uint16_t key;
} Protocol_RemoteInfoTypeDef;

void Protocol_DecodePC(Protocol_PcInfoDownTypeDef *hp, const uint8_t *raw);
void Protocol_EncodePC(uint8_t *raw, const Protocol_PcInfoUpTypeDef *hp);

void Protocol_DecodeJudge(Protocol_JudgeInfoTypeDef *hp, const uint8_t *raw);
void Protocol_EncodeJudge(uint8_t *raw, const Protocol_JudgeInfoTypeDef *hp);

void Protocol_DecodeRemote(Protocol_RemoteInfoTypeDef *hp, const uint8_t *raw);

#endif
