/* 
	裁判系统抽象。

*/

/* Includes ------------------------------------------------------------------*/
#pragma once

/* Include cmsis_os2.h头文件 */
#include "cmsis_os2.h"

#include "user_math.h"


/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
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
	uint8_t place_holder; // TODO
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
} Referee_RobotStatus_t;

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

typedef enum {
	REF_BOT_RED_HERO		= 1,
	REF_BOT_RED_ENGINEER	= 2,
	REF_BOT_RED_INFANTRY_1	= 3,
	REF_BOT_RED_INFANTRY_2	= 4,
	REF_BOT_RED_INFANTRY_3	= 5,
	REF_BOT_RED_DRONE		= 6,
	REF_BOT_RED_SENTRY		= 7,
	REF_BOT_BLU_HERO		= 11,
	REF_BOT_BLU_ENGINEER	= 12,
	REF_BOT_BLU_INFANTRY_1	= 13,
	REF_BOT_BLU_INFANTRY_2	= 14,
	REF_BOT_BLU_INFANTRY_3	= 15,
	REF_BOT_BLU_DRONE		= 16,
	REF_BOT_BLU_SENTRY		= 17,
} Referee_RobotID_t;

typedef enum {
	REF_CL_RED_HERO			= 0x0101,
	REF_CL_RED_ENGINEER		= 0x0102,
	REF_CL_RED_INFANTRY_1	= 0x0103,
	REF_CL_RED_INFANTRY_2	= 0x0104,
	REF_CL_RED_INFANTRY_3	= 0x0105,
	REF_CL_RED_DRONE		= 0x0106,
	REF_CL_BLU_HERO			= 0x0165,
	REF_CL_BLU_ENGINEER		= 0x0166,
	REF_CL_BLU_INFANTRY_1	= 0x0167,
	REF_CL_BLU_INFANTRY_2	= 0x0168,
	REF_CL_BLU_INFANTRY_3	= 0x0169,
	REF_CL_BLU_DRONE		= 0x016A,
} Referee_ClientID_t;

typedef enum {
	REF_STDNT_CMD_ID_UI_DEL		= 0x0100,
	REF_STDNT_CMD_ID_UI_DRAW1	= 0x0101,
	REF_STDNT_CMD_ID_UI_DRAW2	= 0x0102,
	REF_STDNT_CMD_ID_UI_DRAW5	= 0x0103,
	REF_STDNT_CMD_ID_UI_DRAW7	= 0x0104,
	REF_STDNT_CMD_ID_UI_STR		= 0x0110,
	
	REF_STDNT_CMD_ID_CUSTOM		= 0x0200,
} Referee_StudentCMDID_t;

typedef __packed struct {
	Referee_StudentCMDID_t data_cmd_id;
	uint16_t id_sender;
	uint16_t id_receiver;
	uint8_t *data;
} Referee_InterStudent_t;

typedef __packed struct {
	uint8_t place_holder;
} Referee_InterStudent_Custom_t;

/* op: 0: no op; 1: del layer; 2: del all */
typedef __packed struct {
	uint8_t op;
	uint8_t num_layer;
} Referee_InterStudent_UIDel_t;

typedef __packed struct {
	uint8_t name[3];
	uint8_t type_op:3;
	uint8_t type_ele:3;
	uint8_t layer:4;
	uint8_t color:4;
	uint16_t angle_start:9;
	uint16_t angle_end:9;
	uint16_t width:10;
	uint16_t x_start:11;
	uint16_t y_start:11;
	uint16_t radius:10;
	uint16_t x_end:11;
	uint16_t y_end:11;
} Referee_InterStudent_UIEle_t;

typedef __packed struct {
	osThreadId_t thread_alert;

	Referee_GameStatus_t			game_status;
	Referee_GameResult_t			game_result;
	Referee_GameRobotHP_t			game_robot_hp;
	Referee_DartStatus_t			dart_status;
	Referee_ICRAZoneStatus_t		icra_zone;
	Referee_FieldEvents_t			field_event;
	Referee_SupplyAction_t			supply_action;
	Referee_Warning_t				warning;
	Referee_DartCountdown_t			dart_countdown;
	Referee_RobotStatus_t			robot_status;
	Referee_PowerHeat_t				power_heat;
	Referee_RobotPos_t				robot_pos;
	Referee_RobotBuff_t				robot_buff;
	Referee_DroneEnergy_t			drone_energy;
	Referee_RobotDamage_t			robot_danage;
	Referee_ShootData_t				shoot_data;
	Referee_BulletRemain_t			bullet_remain;
	Referee_RFID_t					rfid;
	Referee_DartClient_t			dart_client;
	Referee_InterStudent_Custom_t	custom;
} Referee_t;


/* Exported functions prototypes ---------------------------------------------*/

int8_t Referee_Init(Referee_t *dr16, osThreadId_t thread_alert);
Referee_t *Referee_GetDevice(void);
int8_t Referee_Restart(void);

int8_t Referee_StartReceiving(Referee_t *dr16);
int8_t Referee_Parse(Referee_t *ref, const uint8_t *buf);
