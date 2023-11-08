/*
  裁判系统抽象。
*/

#pragma once

#include <device.hpp>

#include "comp_ui.hpp"

#define GAME_HEAT_INCREASE_42MM (100.0f) /* 每发射一颗42mm弹丸增加100热量 */
#define GAME_HEAT_INCREASE_17MM (10.0f) /* 每发射一颗17mm弹丸增加10热量 */

#define GAME_CHASSIS_MAX_POWER_WO_REF 40.0f /* 裁判系统离线时底盘最大功率 */
#define REF_UI_BOX_UP_OFFSET (4)
#define REF_UI_BOX_BOT_OFFSET (-14)

#define REF_UI_RIGHT_START_W (0.85f)

#define REF_UI_RIGHT_FRIC (0.60f)
#define REF_UI_RIGHT_TRIC (0.60f)

#define REF_UI_MODE_LINE1_H (0.7f)
#define REF_UI_MODE_LINE2_H (0.68f)
#define REF_UI_MODE_LINE3_H (0.66f)
#define REF_UI_MODE_LINE4_H (0.64f)
#define REF_UI_MODE_LINE_BOTTOM_H (0.1f)

#define REF_UI_MODE_OFFSET_1_LEFT (-6)
#define REF_UI_MODE_OFFSET_1_RIGHT (44)
#define REF_UI_MODE_OFFSET_2_LEFT (54)
#define REF_UI_MODE_OFFSET_2_RIGHT (102)
#define REF_UI_MODE_OFFSET_3_LEFT (114)
#define REF_UI_MODE_OFFSET_3_RIGHT (162)
#define REF_UI_MODE_OFFSET_4_LEFT (174)
#define REF_UI_MODE_OFFSET_4_RIGHT (222)
namespace Device {
class Referee {
 public:
  typedef struct __attribute__((packed)) {
    uint8_t sof;
    uint16_t data_length;
    uint8_t seq;
    uint8_t crc8;
  } Header;

  typedef enum {
    OFFLINE = 0,
    RUNNING,
  } Status;

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
    REF_CMD_ID_ROBOT_POS_TO_SENTRY = 0X020B,
    REF_CMD_ID_RADAR_MARK = 0X020C,
    REF_CMD_ID_INTER_STUDENT = 0x0301,
    REF_CMD_ID_INTER_STUDENT_CUSTOM = 0x0302,
    REF_CMD_ID_CLIENT_MAP = 0x0303,
    REF_CMD_ID_KEYBOARD_MOUSE = 0x0304,
    REF_CMD_ID_CUSTOM_KEYBOARD_MOUSE = 0X0306,
    REF_CMD_ID_SENTRY_POS_DATA = 0x0307,
  } CommandID;

  typedef enum {
    REF_GAME_TYPE_RMUC,
    REF_GAME_TYPE_RMUT,
    REF_GAME_TYPE_RMUA,
    REF_GAME_TYPE_RMUL_3V3,
    REF_GAME_TYPE_RMUL_1V1,
  } GameType;

  typedef enum {
    REF_GAME_START = 28,
    REF_ATTACKED,
    GIMBALCONTROLLER_CONTROL,
  } SentryControl;

  typedef struct __attribute__((packed)) {
    uint8_t game_type : 4;
    uint8_t game_progress : 4;
    uint16_t stage_remain_time;
    uint64_t sync_time_stamp;
  } GameStatus;

  typedef struct __attribute__((packed)) {
    uint8_t winner;
  } GameResult;

  typedef struct __attribute__((packed)) {
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
  } RobotHP;

  typedef struct __attribute__((packed)) {
    uint8_t dart_belong;
    uint16_t stage_remain_time;
  } DartStatus;

  typedef struct __attribute__((packed)) {
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
  } IcraZoneStatus;

  typedef struct __attribute__((packed)) {
    uint8_t copter_pad : 2;
    uint8_t energy_mech : 2;
    uint8_t virtual_shield : 1;
    uint32_t res : 27;
  } FieldEvents;

  typedef struct __attribute__((packed)) {
    uint8_t supply_id;
    uint8_t robot_id;
    uint8_t supply_step;
    uint8_t supply_sum;
  } SupplyAction;

  typedef struct __attribute__((packed)) {
    uint8_t level;
    uint8_t robot_id;
  } Warning;

  typedef struct __attribute__((packed)) {
    uint8_t countdown;
  } DartCountdown;

  typedef struct __attribute__((packed)) {
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
  } RobotStatus;

  typedef struct __attribute__((packed)) {
    uint16_t chassis_volt;
    uint16_t chassis_amp;
    float chassis_watt;
    uint16_t chassis_pwr_buff;
    uint16_t launcher_id1_17_heat;
    uint16_t launcher_id2_17_heat;
    uint16_t launcher_42_heat;
  } PowerHeat;

  typedef struct __attribute__((packed)) {
    float x;
    float y;
    float z;
    float yaw;
  } RobotPOS;

  typedef struct __attribute__((packed)) {
    uint8_t healing : 1;
    uint8_t cooling_acc : 1;
    uint8_t defense_buff : 1;
    uint8_t attack_buff : 1;
    uint8_t res : 4;
  } RobotBuff;

  typedef struct __attribute__((packed)) {
    uint8_t attack_countdown;
  } DroneEnergy;

  typedef struct __attribute__((packed)) {
    uint8_t armor_id : 4;
    uint8_t damage_type : 4;
  } RobotDamage;

  typedef struct __attribute__((packed)) {
    uint8_t bullet_type;
    uint8_t launcherer_id;
    uint8_t bullet_freq;
    float bullet_speed;
  } LauncherData;

  typedef struct __attribute__((packed)) {
    uint16_t bullet_17_remain;
    uint16_t bullet_42_remain;
    uint16_t coin_remain;
  } BulletRemain;

  typedef struct __attribute__((packed)) {
    uint8_t base : 1;
    uint8_t high_ground : 1;
    uint8_t energy_mech : 1;
    uint8_t slope : 1;
    uint8_t outpose : 1;
    uint8_t resource : 1;
    uint8_t healing_card : 1;
    uint32_t res : 24;
  } RFID;

  typedef struct __attribute__((packed)) {
    uint8_t opening;
    uint8_t target;
    uint8_t target_changable_countdown;
    uint8_t dart1_speed;
    uint8_t dart2_speed;
    uint8_t dart3_speed;
    uint8_t dart4_speed;
    uint16_t last_dart_launch_time;
    uint16_t operator_cmd_launch_time;
  } DartClient;

  typedef struct __attribute__((packed)) {
    float position_x;
    float position_y;
    float position_z;
    uint8_t commd_keyboard;
    uint16_t robot_id;
  } ClientMap;

  typedef struct __attribute__((packed)) {
    int16_t mouse_x;
    int16_t mouse_y;
    int16_t mouse_wheel;
    int8_t button_l;
    int8_t button_r;
    uint16_t keyboard_value;
    uint16_t res;
  } KeyboardMouse;
  typedef struct __attribute__((packed)) {
    uint8_t intention;
    uint16_t start_position_x;
    uint16_t start_position_y;
    std::array<int8_t, 49> delta_x;
    std::array<int8_t, 49> delta_y;
  } SentryPosition;

  typedef struct __attribute__((packed)) {
    std::array<uint8_t, 30> data;
  } CustomController;
  typedef struct __attribute__((packed)) {
    float hero_x;
    float hero_y;
    float engineer_x;
    float engineer_y;
    float standard_3_x;
    float standard_3_y;
    float standard_4_x;
    float standard_4_y;
    float standard_5_x;
    float standard_5_y;
  } RobotPosForSentry;
  typedef struct __attribute__((packed)) {
    uint8_t mark_hero_progress;
    uint8_t mark_engineer_progress;
    uint8_t mark_standard_3_progress;
    uint8_t mark_standard_4_progress;
    uint8_t mark_standard_5_progress;
    uint8_t mark_sentry_progress;
  } RadarMarkProgress;
  typedef struct __attribute__((packed)) {
    uint16_t key_value;
    uint16_t x_position : 12;
    uint16_t mouse_left : 4;
    uint16_t y_position : 12;
    uint16_t mouse_right : 4;
    uint16_t reserved;
  } CustomKeyMouseData;

  typedef uint16_t Tail;

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
  } RobotID;

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
  } ClientID;

  typedef enum {
    REF_STDNT_CMD_ID_UI_DEL = 0x0100,
    REF_STDNT_CMD_ID_UI_DRAW1 = 0x0101,
    REF_STDNT_CMD_ID_UI_DRAW2 = 0x0102,
    REF_STDNT_CMD_ID_UI_DRAW5 = 0x0103,
    REF_STDNT_CMD_ID_UI_DRAW7 = 0x0104,
    REF_STDNT_CMD_ID_UI_STR = 0x0110,
    REF_STDNT_CMD_ID_CUSTOM = 0x0200,
  } CMDID;

  typedef struct __attribute__((packed)) {
    CMDID cmd_id;
    uint16_t id_sender;
    uint16_t id_receiver;
  } InterStudentHeader;

  typedef struct {
    Status status;
    GameStatus game_status;
    GameResult game_result;
    RobotHP game_robot_hp;
    DartStatus dart_status;
    IcraZoneStatus icra_zone;
    FieldEvents field_event;
    SupplyAction supply_action;
    Warning warning;
    DartCountdown dart_countdown;
    RobotStatus robot_status;
    PowerHeat power_heat;
    RobotPOS robot_pos;
    RobotBuff robot_buff;
    DroneEnergy drone_energy;
    RobotDamage robot_damage;
    LauncherData launcher_data;
    BulletRemain bullet_remain;
    RFID rfid;
    DartClient dart_client;
    ClientMap client_map;
    KeyboardMouse keyboard_mouse;
    SentryPosition sentry_postion;
    CustomController custom_controller;
    RobotPosForSentry robot_pos_for_snetry;
    RadarMarkProgress radar_mark_progress;
    CustomKeyMouseData custom_key_mouse_data;
  } Data;

  typedef struct __attribute__((packed)) {
    Header frame_header;
    uint16_t cmd_id;
    Referee::InterStudentHeader student_header;
    std::array<Component::UI::Ele, 1> ele_data;
    uint16_t crc16;
  } UIElePack_1;

  typedef struct __attribute__((packed)) {
    Header frame_header;
    uint16_t cmd_id;
    Referee::InterStudentHeader student_header;
    std::array<Component::UI::Ele, 2> ele_data;
    uint16_t crc16;
  } UIElePack_2;

  typedef struct __attribute__((packed)) {
    Header frame_header;
    uint16_t cmd_id;
    Referee::InterStudentHeader student_header;
    std::array<Component::UI::Ele, 5> ele_data;
    uint16_t crc16;
  } UIElePack_5;

  typedef struct __attribute__((packed)) {
    Header frame_header;
    uint16_t cmd_id;
    Referee::InterStudentHeader student_header;
    std::array<Component::UI::Ele, 7> ele_data;
    uint16_t crc16;
  } UIElePack_7;

  typedef struct __attribute__((packed)) {
    Header frame_header;
    uint16_t cmd_id;
    Referee::InterStudentHeader student_header;
    Component::UI::Del del_data;
    uint16_t crc16;
  } UIDelPack;

  typedef struct __attribute__((packed)) {
    Header frame_header;
    uint16_t cmd_id;
    Referee::InterStudentHeader student_header;
    Component::UI::Str str_data;
    uint16_t crc16;
  } UIStringPack;

  union UIPack {
    UIElePack_7 ele_7;
    UIStringPack str;
    UIDelPack del;
    struct __attribute__((packed)) {
      Header frame_header;
      uint16_t cmd_id;
      Referee::InterStudentHeader student_header;
    } raw;
  };

  Referee();

  bool UIStackEmpty();

  void Offline();

  bool StartRecv();

  void Prase();

  bool UpdateUI();

  static bool AddUI(Component::UI::Ele ui_data);
  static bool AddUI(Component::UI::Del ui_data);
  static bool AddUI(Component::UI::Str ui_data);

  static float UIGetHeight() { return 1080.0f; }
  static float UIGetWidth() { return 1920.0f; }

  bool PackUI();

  bool StartTrans();

  void SetUIHeader(InterStudentHeader &header, const CMDID CMD_ID,
                   RobotID robot_id);

  void SetPacketHeader(Referee::Header &header, uint16_t data_length);

 private:
  System::Semaphore raw_ready_ = System::Semaphore(false);
  System::Semaphore packet_sent_ = System::Semaphore(true);

  System::Thread recv_thread_;
  System::Thread trans_thread_;

  Message::Topic<Data> ref_data_tp_ = Message::Topic<Data>("referee");

  System::Queue<Component::UI::Ele> ele_data_ =
      System::Queue<Component::UI::Ele>(10);

  System::Queue<Component::UI::Str> string_data_ =
      System::Queue<Component::UI::Str>(10);

  System::Queue<Component::UI::Del> del_data_ =
      System::Queue<Component::UI::Del>(10);

  System::Queue<Component::UI::Ele> static_ele_data_ =
      System::Queue<Component::UI::Ele>(10);

  System::Queue<Component::UI::Str> static_string_data_ =
      System::Queue<Component::UI::Str>(10);

  System::Queue<Component::UI::Del> static_del_data_ =
      System::Queue<Component::UI::Del>(10);

  System::Semaphore ui_lock_ = System::Semaphore(true);

  Data ref_data_;

  Data last_data_;

  Message::Event event_;

  static UIPack ui_pack_;

  static Referee *self_;
};
}  // namespace Device
