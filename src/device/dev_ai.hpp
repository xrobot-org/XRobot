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

  struct {
    System::Semaphore data_ready_;
  } sem;

  Component::Type::Quaternion quat_;
  RefForAI ref_;
  Component::CMD::Host cmd_;
  Component::Type::Eulr eulr_;

  AI();

  bool StartRecv();

  bool PraseHost();

  bool StartTrans();

  bool Offline();

  bool PackMCU();

  bool PackRef();

  void PraseRef(Device::Referee::Data *ref);

  bool PackCMD();

  bool ref_updated_;
  bool online_;
  uint32_t last_online_time_;

  Protocol_DownPackage_t form_host;

  struct {
    RefereePckage ref_;
    MCUPckage mcu_;
  } to_host;

  System::Thread thread_;
};
}  // namespace Device
