#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	GIMBAL_MODE_AIM,
	GIMBAL_MODE_PATROL,
} Gimbal_Mode_t;

typedef enum {
	SHOOT_MODE_IDLE,
	SHOOT_MODE_SLOW,
	SHOOT_MODE_FAST,
	SHOOT_MODE_SUICIDE,
} Shoot_Mode_t;

typedef enum {
	CHASSIS_MODE_PATROL,
	CHASSIS_MODE_DODGE,
} Chassis_Mode_t;

typedef __packed struct {
	Gimbal_Mode_t gimbal_mode;
	Shoot_Mode_t shoot_mode;
	Chassis_Mode_t chassis_mode;
} Protocol_PcDown_t;

typedef __packed struct {
	Gimbal_Mode_t gimbal_mode;
	Shoot_Mode_t shoot_mode;
	Chassis_Mode_t chassis_mode;
} Protocol_PcUp_t;

typedef __packed struct {
	int16_t i;
} Protocol_JudgeInfo_t;

void Protocol_DecodePC(Protocol_PcDown_t *hpc, const uint8_t *raw);
void Protocol_EncodePC(uint8_t *raw, const Protocol_PcDown_t *hpc);

void Protocol_DecodeJudge(Protocol_JudgeInfo_t *hjudge, const uint8_t *raw);
void Protocol_EncodeJudge(uint8_t *raw, const Protocol_JudgeInfo_t *hjudge);
