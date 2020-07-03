/* 
	裁判系统抽象。

*/
#pragma once

#include "user_math.h"

#define HEADER_SOF 0xA5
#define REF_PROTOCOL_FRAME_MAX_SIZE         128

#define REF_PROTOCOL_HEADER_SIZE            sizeof(frame_header_struct_t)
#define REF_PROTOCOL_CMD_SIZE               2
#define REF_PROTOCOL_CRC16_SIZE             2
#define REF_HEADER_CRC_LEN                  (REF_PROTOCOL_HEADER_SIZE + REF_PROTOCOL_CRC16_SIZE)
#define REF_HEADER_CRC_CMDID_LEN            (REF_PROTOCOL_HEADER_SIZE + REF_PROTOCOL_CRC16_SIZE + sizeof(uint16_t))
#define REF_HEADER_CMDID_LEN                (REF_PROTOCOL_HEADER_SIZE + sizeof(uint16_t))

#pragma pack(push, 1)

#define REFEREE_SOF (0xA5)


typedef  struct {
	uint8_t sof;
	uint16_t data_length;
	uint8_t seq;
	uint8_t crc8;
} Referee_FrameHeader_t;

typedef enum {
	GAME_STATE_CMD_ID					= 0x0001,
	GAME_RESULT_CMD_ID					= 0x0002,
	GAME_ROBOT_HP_CMD_ID				= 0x0003,
	DART_STATUS_CMD_ID					= 0x0004,
	GAME_RESULT_CMD_ID					= 0x0005,
	FIELD_EVENTS_CMD_ID					= 0x0101,
	SUPPLY_PROJECTILE_ACTION_CMD_ID		= 0x0102,
	REFEREE_WARNING_CMD_ID				= 0x0104,
	REFEREE_WARNING_CMD_ID				= 0x0105,
	ROBOT_STATE_CMD_ID					= 0x0201,
	POWER_HEAT_DATA_CMD_ID				= 0x0202,
	ROBOT_POS_CMD_ID					= 0x0203,
	BUFF_MUSK_CMD_ID					= 0x0204,
	AERIAL_ROBOT_ENERGY_CMD_ID			= 0x0205,
	ROBOT_HURT_CMD_ID					= 0x0206,
	SHOOT_DATA_CMD_ID					= 0x0207,
	BULLET_REMAINING_CMD_ID				= 0x0208,
	BULLET_REMAINING_CMD_ID				= 0x0208,
	BULLET_REMAINING_CMD_ID				= 0x0208,
	STUDENT_INTERACTIVE_DATA_CMD_ID		= 0x0301,
} Referee_CMDID_t;

typedef __packed struct {
	uint8_t game_type:4;
	uint8_t game_progress:4;
	uint16_t stage_remain_time;
} Referee_GameStatus_t;

typedef __packed struct {
	uint8_t winner;
} Referee_GameResult_t;

typedef __packed struct {
	uint16_t red_1;
	uint16_t red_2;
	uint16_t red_3;
	uint16_t red_4;
	uint16_t red_5;
	uint16_t red_6;
	uint16_t red_7;
	uint16_t red_outpose;
	uint16_t red_base;
	uint16_t blue_1;
	uint16_t blue_2;
	uint16_t blue_3;
	uint16_t blue_4;
	uint16_t blue_5;
	uint16_t blue_6;
	uint16_t blue_7;
	uint16_t blue_outpose;
	uint16_t blue_base;
} Referee_GameRobotHP_t;

typedef __packed struct {
	uint8_t dart_belong;
	uint16_t stage_remain_time;
} Referee_DartStatus_t;

typedef __packed struct {
	// TODO
} Referee_ICRAZoneStatus_t;

typedef __packed struct {
	uint8_t copter_pad:2;
	uint8_t energy_mech:2;
	uint8_t virtual_shield:1;
	uint32_t res:27;
} Referee_FieldEvents_t;

typedef __packed struct {
	uint8_t supply_id;
	uint8_t robot_id;
	uint8_t supply_step;
	uint8_t supply_sum;
} Referee_SupplyAction_t;

typedef __packed struct {
	uint8_t level;
	uint8_t robot_id;
} Referee_Warning_t;

typedef __packed struct {
	uint8_t countdown;
} Referee_DartCountdown_t;

typedef __packed struct {
	uint8_t robot_id;
	uint8_t robot_level;
	uint16_t remain_hp;
	uint16_t max_hp;
	uint16_t shoot_17_cooling_rate;
	uint16_t shoot_17_heat_limit;
	uint16_t shoot_42_cooling_rate;
	uint16_t shoot_42_heat_limit;
	uint16_t shoot_17_speed_limit;
	uint16_t shoot_42_speed_limit;
	uint16_t chassis_power_limit;
	uint8_t power_gimbal_output:1;
	uint8_t power_chassis_output:1;
	uint8_t power_shoot_output:1;
} Referee_RobotState_t;

typedef __packed struct {
	uint16_t chassis_volt;
	uint16_t chassis_amp;
	float32_t chassis_watt;
	uint16_t chassis_pwr_buff;
	uint16_t shoot_17_heat;
	uint16_t shoot_42_heat;
	uint16_t shoot_17_opt_heat;
} Referee_PowerHeat_t;

typedef __packed struct {
	float32_t x;
	float32_t y;
	float32_t z;
	float32_t yaw;
} Referee_RobotPos_t;

typedef __packed struct {
	uint8_t healing:1;
	uint8_t cooling_acc:1;
	uint8_t defense_buff:1;
	uint8_t attack_buff:1;
	uint8_t res:4;
} Referee_RobotBuff_t;

typedef __packed struct {
	uint16_t energy_point;
	uint8_t attack_countdown;
} Referee_DroneEnergy_t;

typedef __packed struct {
	uint8_t armor_id:4;
	uint8_t damage_type:4;
} Referee_RobotDamage_t;

typedef __packed struct {
	uint8_t bullet_type;
	uint8_t bullet_freq;
	float32_t bullet_speed;
} Referee_ShootData_t;	

typedef __packed struct {
	uint16_t bullet_remain;
} Referee_BulletRemain_t;

typedef __packed struct {
	uint8_t base:1;
	uint8_t high_ground:1;
	uint8_t energy_mech:1;
	uint8_t slope:1;
	uint8_t outpose:1;
	uint8_t resource:1;
	uint8_t healing_card:1;
	uint32_t res:24;
} Referee_RFID_t;

typedef __packed struct {
	uint8_t opening;
	uint8_t target;
	uint8_t target_changable_countdown;
	uint8_t dart1_speed;
	uint8_t dart2_speed;
	uint8_t dart3_speed;
	uint8_t dart4_speed;
	uint16_t last_dart_launch_time;
	uint16_t operator_cmd_launch_time;
} Referee_DartClient_t;

typedef __packed struct {
  frame_header_struct_t *p_header;
  uint16_t data_len;
  uint8_t protocol_packet[REF_PROTOCOL_FRAME_MAX_SIZE];
  unpack_step_e unpack_step;
  uint16_t index;
} unpack_data_t;

