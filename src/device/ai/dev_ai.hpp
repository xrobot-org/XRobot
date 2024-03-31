#pragma once

#include <device.hpp>

#include "comp_cmd.hpp"
#include "dev_ahrs.hpp"
#include "dev_referee.hpp"
#include "protocol.h"

namespace Device {
class AI {
 public:
  typedef struct __attribute__((packed)) {
    uint8_t id;
    Protocol_UpPackageReferee_t package;
  } RefereePckage;

  typedef struct __attribute__((packed)) {
    uint8_t id;
    Protocol_UpPackageMCU_t package;
  } MCUPckage;

  typedef struct {
    uint8_t game_type;
    Device::Referee::Status status;
    uint8_t team;
    uint8_t robot_id;
    uint8_t robot_buff;
    uint32_t ball_speed;
    uint32_t max_hp;
    uint32_t hp;
  } RefForAI;

  typedef enum {
    AI_OFFLINE = 128,
    AI_FIND_TARGET,
    AI_AUTOPATROL,
    AI_TURN,
    AI_FIRE_COMMAND,
  } AIControlData;

  AI();

  bool StartRecv();

  bool PraseHost();

  bool StartTrans();

  bool Offline();

  bool PackMCU();

  bool PackRef();

  void PraseRef();

  bool PackCMD();

 private:
  bool ref_updated_ = false;

  uint32_t last_online_time_ = 0;

  Protocol_DownPackage_t from_host_{};

  uint8_t notice_;

  struct {
    RefereePckage ref{};
    MCUPckage mcu{};
  } to_host_;

  RefForAI ref_{};

  System::Thread thread_;

  System::Semaphore data_ready_;

  Message::Event event_;

  Message::Topic<Component::CMD::Data> cmd_tp_;

  Component::CMD::Data cmd_{};

  struct {
    float yaw; /* 偏航角（Yaw angle） */
    float pit; /* 俯仰角（Pitch angle） */
    float rol; /* 翻滚角（Roll angle） */
  } last_eulr_;

  Component::Type::Quaternion quat_{};
  Device::Referee::Data raw_ref_{};
};
}  // namespace Device
