/*
  裁判系统抽象。
*/

#pragma once

#include <stdbool.h>

#include "FreeRTOS.h"
#include "comp_cmd.h"
#include "comp_ui.h"
#include "comp_utils.h"
#include "dev.h"
#include "semphr.h"
#include "timers.h"

typedef struct __packed {
  uint8_t sof;
  uint16_t data_length;
  uint8_t seq;
  uint8_t crc8;
} referee_header_t;

typedef enum {
  REF_STATUS_OFFLINE = 0,
  REF_STATUS_RUNNING,
} referee_status_t;

typedef enum {
  REF_CMD_ID_GAME_STATUS = 0x0001,
  REF_CMD_ID_GAME_RESULT = 0x0002,
  REF_CMD_ID_GAME_ROBOT_HP = 0x0003,
  REF_CMD_ID_DART_STATUS = 0x0004,
  REF_CMD_ID_ICRA_ZONE_STATUS = 0x0005,
  REF_CMD_ID_FIELD_EVENTS = 0x0101,
  REF_CMD_ID_SUPPLY_ACTION = 0x0102,
  REF_CMD_ID_WARNING = 0x0104,
  REF_CMD_ID_DART_COUNTDOWN = 0x0105,
  REF_CMD_ID_ROBOT_STATUS = 0x0201,
  REF_CMD_ID_POWER_HEAT_DATA = 0x0202,
  REF_CMD_ID_ROBOT_POS = 0x0203,
  REF_CMD_ID_ROBOT_BUFF = 0x0204,
  REF_CMD_ID_DRONE_ENERGY = 0x0205,
  REF_CMD_ID_ROBOT_DMG = 0x0206,
  REF_CMD_ID_LAUNCHER_DATA = 0x0207,
  REF_CMD_ID_BULLET_REMAINING = 0x0208,
  REF_CMD_ID_RFID = 0x0209,
  REF_CMD_ID_DART_CLIENT = 0x020A,
  REF_CMD_ID_INTER_STUDENT = 0x0301,
  REF_CMD_ID_INTER_STUDENT_CUSTOM = 0x0302,
  REF_CMD_ID_CLIENT_MAP = 0x0303,
  REF_CMD_ID_KEYBOARD_MOUSE = 0x0304,
} referee_cmd_id_t;

typedef enum{
  REF_GAME_TYPE_RMUC,
  REF_GAME_TYPE_RMUT,
  REF_GAME_TYPE_RMUA,
  REF_GAME_TYPE_RMUL_3V3,
  REF_GAME_TYPE_RMUL_1V1,
} referee_game_type_t;

typedef struct __packed {
  uint8_t game_type : 4;
  uint8_t game_progress : 4;
  uint16_t stage_remain_time;
  uint64_t sync_time_stamp;
} referee_game_status_t;

typedef struct __packed {
  uint8_t winner;
} referee_game_result_t;

typedef struct __packed {
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
} referee_game_robot_hp_t;

typedef struct __packed {
  uint8_t dart_belong;
  uint16_t stage_remain_time;
} referee_dart_status_t;

typedef struct __packed {
  uint8_t f1_status : 1;
  uint8_t f1_buff_status : 3;
  uint8_t f2_status : 1;
  uint8_t f2_buff_status : 3;
  uint8_t f3_status : 1;
  uint8_t f3_buff_status : 3;
  uint8_t f4_status : 1;
  uint8_t f4_buff_status : 3;
  uint8_t f5_status : 1;
  uint8_t f5_buff_status : 3;
  uint8_t f6_status : 1;
  uint8_t f6_buff_status : 3;
  uint16_t red1_bullet_remain;
  uint16_t red2_bullet_remain;
  uint16_t blue1_bullet_remain;
  uint16_t blue2_bullet_remain;
} referee_icra_zone_status_t;

typedef struct __packed {
  uint8_t copter_pad : 2;
  uint8_t energy_mech : 2;
  uint8_t virtual_shield : 1;
  uint32_t res : 27;
} referee_field_events_t;

typedef struct __packed {
  uint8_t supply_id;
  uint8_t robot_id;
  uint8_t supply_step;
  uint8_t supply_sum;
} referee_supply_action_t;

typedef struct __packed {
  uint8_t level;
  uint8_t robot_id;
} referee_warning_t;

typedef struct __packed {
  uint8_t countdown;
} referee_dart_countdown_t;

typedef struct __packed {
  uint8_t robot_id;
  uint8_t robot_level;
  uint16_t remain_hp;
  uint16_t max_hp;
  uint16_t launcher_id1_17_cooling_rate;
  uint16_t launcher_id1_17_heat_limit;
  uint16_t launcher_id1_17_speed_limit;
  uint16_t launcher_id2_17_cooling_rate;
  uint16_t launcher_id2_17_heat_limit;
  uint16_t launcher_id2_17_speed_limit;
  uint16_t launcher_42_cooling_rate;
  uint16_t launcher_42_heat_limit;
  uint16_t launcher_42_speed_limit;
  uint16_t chassis_power_limit;
  uint8_t power_gimbal_output : 1;
  uint8_t power_chassis_output : 1;
  uint8_t power_launcher_output : 1;
} referee_robot_status_t;

typedef struct __packed {
  uint16_t chassis_volt;
  uint16_t chassis_amp;
  float chassis_watt;
  uint16_t chassis_pwr_buff;
  uint16_t launcher_id1_17_heat;
  uint16_t launcher_id2_17_heat;
  uint16_t launcher_42_heat;
} referee_power_heat_t;

typedef struct __packed {
  float x;
  float y;
  float z;
  float yaw;
} referee_robot_pos_t;

typedef struct __packed {
  uint8_t healing : 1;
  uint8_t cooling_acc : 1;
  uint8_t defense_buff : 1;
  uint8_t attack_buff : 1;
  uint8_t res : 4;
} referee_robot_buff_t;

typedef struct __packed {
  uint8_t attack_countdown;
} referee_drone_energy_t;

typedef struct __packed {
  uint8_t armor_id : 4;
  uint8_t damage_type : 4;
} referee_robot_damage_t;

typedef struct __packed {
  uint8_t bullet_type;
  uint8_t launcherer_id;
  uint8_t bullet_freq;
  float bullet_speed;
} referee_launcher_data_t;

typedef struct __packed {
  uint16_t bullet_17_remain;
  uint16_t bullet_42_remain;
  uint16_t coin_remain;
} referee_bullet_remain_t;

typedef struct __packed {
  uint8_t base : 1;
  uint8_t high_ground : 1;
  uint8_t energy_mech : 1;
  uint8_t slope : 1;
  uint8_t outpose : 1;
  uint8_t resource : 1;
  uint8_t healing_card : 1;
  uint32_t res : 24;
} referee_rfid_t;

typedef struct __packed {
  uint8_t opening;
  uint8_t target;
  uint8_t target_changable_countdown;
  uint8_t dart1_speed;
  uint8_t dart2_speed;
  uint8_t dart3_speed;
  uint8_t dart4_speed;
  uint16_t last_dart_launch_time;
  uint16_t operator_cmd_launch_time;
} referee_dart_client_t;

typedef struct __packed {
  float position_x;
  float position_y;
  float position_z;
  uint8_t commd_keyboard;
  uint16_t robot_id;
} referee_client_map_t;

typedef struct __packed {
  int16_t mouse_x;
  int16_t mouse_y;
  int16_t mouse_wheel;
  int8_t button_l;
  int8_t button_r;
  uint16_t keyboard_value;
  uint16_t res;
} referee_keyboard_mouse_t;

typedef uint16_t referee_tail_t;

typedef enum {
  REF_BOT_RED_HERO = 1,
  REF_BOT_RED_ENGINEER = 2,
  REF_BOT_RED_INFANTRY_1 = 3,
  REF_BOT_RED_INFANTRY_2 = 4,
  REF_BOT_RED_INFANTRY_3 = 5,
  REF_BOT_RED_DRONE = 6,
  REF_BOT_RED_SENTRY = 7,
  REF_BOT_RED_RADER = 9,
  REF_BOT_BLU_HERO = 101,
  REF_BOT_BLU_ENGINEER = 102,
  REF_BOT_BLU_INFANTRY_1 = 103,
  REF_BOT_BLU_INFANTRY_2 = 104,
  REF_BOT_BLU_INFANTRY_3 = 105,
  REF_BOT_BLU_DRONE = 106,
  REF_BOT_BLU_SENTRY = 107,
  REF_BOT_BLU_RADER = 109,
} referee_robot_id_t;

typedef enum {
  REF_CL_RED_HERO = 0x0101,
  REF_CL_RED_ENGINEER = 0x0102,
  REF_CL_RED_INFANTRY_1 = 0x0103,
  REF_CL_RED_INFANTRY_2 = 0x0104,
  REF_CL_RED_INFANTRY_3 = 0x0105,
  REF_CL_RED_DRONE = 0x0106,
  REF_CL_BLU_HERO = 0x0165,
  REF_CL_BLU_ENGINEER = 0x0166,
  REF_CL_BLU_INFANTRY_1 = 0x0167,
  REF_CL_BLU_INFANTRY_2 = 0x0168,
  REF_CL_BLU_INFANTRY_3 = 0x0169,
  REF_CL_BLU_DRONE = 0x016A,
} referee_client_id_t;

typedef enum {
  REF_STDNT_CMD_ID_UI_DEL = 0x0100,
  REF_STDNT_CMD_ID_UI_DRAW1 = 0x0101,
  REF_STDNT_CMD_ID_UI_DRAW2 = 0x0102,
  REF_STDNT_CMD_ID_UI_DRAW5 = 0x0103,
  REF_STDNT_CMD_ID_UI_DRAW7 = 0x0104,
  REF_STDNT_CMD_ID_UI_STR = 0x0110,

  REF_STDNT_CMD_ID_CUSTOM = 0x0200,
} referee_student_cmd_id_t;

typedef struct __packed {
  referee_student_cmd_id_t cmd_id;
  uint16_t id_sender;
  uint16_t id_receiver;
} referee_inter_student_header_t;

typedef struct {
  referee_status_t status;
  referee_game_status_t game_status;
  referee_game_result_t game_result;
  referee_game_robot_hp_t game_robot_hp;
  referee_dart_status_t dart_status;
  referee_icra_zone_status_t icra_zone;
  referee_field_events_t field_event;
  referee_supply_action_t supply_action;
  referee_warning_t warning;
  referee_dart_countdown_t dart_countdown;
  referee_robot_status_t robot_status;
  referee_power_heat_t power_heat;
  referee_robot_pos_t robot_pos;
  referee_robot_buff_t robot_buff;
  referee_drone_energy_t drone_energy;
  referee_robot_damage_t robot_danage;
  referee_launcher_data_t launcher_data;
  referee_bullet_remain_t bullet_remain;
  referee_rfid_t rfid;
  referee_dart_client_t dart_client;
  referee_client_map_t client_map;
  referee_keyboard_mouse_t keyboard_mouse;

  ui_t ui;
  /* UI所需信息 */
  ui_cap_t cap_ui;
  ui_chassis_t chassis_ui;
  ui_launcher_t launcher_ui;
  ui_gimbal_t gimbal_ui;
  cmd_ui_t cmd_ui;

  struct {
    uint8_t *data;
    size_t size;
  } packet;

  struct {
    SemaphoreHandle_t ui_fast_refresh;
    SemaphoreHandle_t ui_slow_refresh;
    SemaphoreHandle_t packet_sent;
    SemaphoreHandle_t raw_ready;
  } sem;

  TaskHandle_t thread_alert;

  TimerHandle_t ui_fast_timer_id;
  TimerHandle_t ui_slow_timer_id;
} referee_t;

typedef struct {
  uint8_t game_type;
  referee_status_t status;
  uint8_t team;
} referee_for_ai_t;

typedef struct {
  referee_status_t status;
  float chassis_power_limit;
  float chassis_pwr_buff;
  float chassis_watt;
} referee_for_chassis_t;

typedef struct {
  referee_status_t status;
  referee_power_heat_t power_heat;
  referee_robot_status_t robot_status;
  referee_launcher_data_t launcher_data;
} referee_for_launcher_t;

int8_t referee_init(referee_t *ref, const ui_screen_t *screen);
int8_t referee_restart(void);
void referee_handle_offline(referee_t *ref);

int8_t referee_start_receiving(referee_t *ref);
bool referee_wait_recv_cplt(referee_t *ref, uint32_t timeout);
int8_t referee_parse(referee_t *ref);

uint8_t referee_refresh_ui(referee_t *ref);
int8_t referee_pack_ui_packet(referee_t *ref);
int8_t referee_start_transmit(referee_t *ref);
bool referee_wait_trans_cplt(referee_t *ref, uint32_t timeout);

uint8_t referee_pack_for_chassis(referee_for_chassis_t *c_ref,
                                 const referee_t *ref);
uint8_t referee_pack_for_launcher(referee_for_launcher_t *l_ref,
                                  const referee_t *ref);
uint8_t referee_pack_for_ai(referee_for_ai_t *ai_ref, const referee_t *ref);
