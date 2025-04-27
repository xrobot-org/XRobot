#pragma once

#include "bsp_uart.h"
#include "comp_cmd.hpp"
#include "comp_crc16.hpp"
#include "dev_ahrs.hpp"
#include "dev_referee.hpp"
#include "device.hpp"

#ifndef pi
#define pi 3.1415926535f
#endif

/* clang-format off */
namespace Device {
class AIM {
 public:

  //MCU需要从上位机获取的数据
  typedef struct __attribute__((packed)) RevforAI{
  uint8_t header = 0xA5;
  bool is_fire : 1;
  uint8_t reserved : 1;
  float x;
  float y;
  float z;
  float v_yaw;
  float pitch;
  float yaw;
  uint16_t checksum = 0;
  }RefForAI_t;

  typedef struct __attribute__ ((packed)) TranToAI{
  uint8_t header = 0x5A;
  uint8_t detect_color : 1;  // 0-red 1-blue
  bool reset_tracker : 1;
  uint8_t reserved : 6;
  float current_v; // m/s
  float yaw;
  float pitch;
  float roll;
  float aim_x;
  float aim_y;
  float aim_z;
  uint16_t checksum = 0;
  }TranToAI_t;

  typedef struct {
    uint8_t game_type;
    Device::Referee::Status status;
    uint8_t team;
    uint8_t robot_id;
    uint8_t robot_buff;
    uint32_t ball_speed;
    uint32_t max_hp;
    uint32_t hp;

    uint8_t game_progress;
    uint16_t base_hp;
    uint16_t outpost_hp;
    uint16_t bullet_num;
    uint16_t coin_num;
    uint8_t own_virtual_shield_value;
    float pos_x;
    float pos_y;
    float pos_angle;
    float target_pos_x;
    float target_pos_y;
    uint8_t damaged_armor_id;
  } RefForAI;

  typedef struct
  {
    float yaw;
    float pitch;
    float roll;
  }ToCMD_t;

  typedef enum {
    AI_STOP_FIRE,
    AI_FIRE_COMMAND,
  } AIControlData;

  AIControlData aim_status_;

  RefForAI_t from_host_;
  TranToAI_t to_host_;

/* ----------------------------------------------------------------------------------------------------- */
  AIM();

  //system
  Component::Type::Eulr eulr_;

  Component::CMD::Data cmd_{};

  Referee::SentryDecisionData cmd_for_ref_;
  /* Topic & Event */

  Message::Event event_;

  Message::Topic<Component::CMD::Data> cmd_tp_;

  /* Task Control */
  System::Thread thread_;

  System::Semaphore data_ready_;

  ToCMD_t to_cmd_;

  bool StartRecv();

  bool StartTran();

  bool PraseHost();

  bool PackCMD();

  bool PackMCU();

uint32_t last_online_time_ = 0;
};
}  // namespace Device
