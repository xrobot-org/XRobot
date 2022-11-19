#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "comp_ahrs.hpp"
#include "comp_utils.hpp"
#include "dev.hpp"
#include "dev_referee.hpp"
#include "protocol.h"

namespace Device {
class AI {
 public:
  typedef struct __packed {
    uint8_t id;
    Protocol_UpPackageReferee_t package;
  } RefereePckage;

  typedef struct __packed {
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

  AI();

  bool StartRecv();

  bool PraseHost();

  bool StartTrans();

  bool Offline();

  bool PackMCU();

  bool PackRef();

  void PraseRef();

  bool PackCMD();

  bool ref_updated_;
  float last_online_time_ = 0.0f;

  Protocol_DownPackage_t form_host;

  struct {
    RefereePckage ref_;
    MCUPckage mcu_;
  } to_host;

  RefForAI ref_;

  System::Thread thread_;

  System::Semaphore data_ready_;

  Message::Topic<Component::CMD::Data> cmd_tp_;

  Component::CMD::Data cmd_;

  Component::Type::Quaternion quat_;
  Device::Referee::Data raw_ref_;
};
}  // namespace Device
