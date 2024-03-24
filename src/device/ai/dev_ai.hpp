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
    AI_ONLINE,
    IS_INVALID_AMING_DATA,
    IS_USEFUL_AMING_DATA,
    AI_FIND_TARGET,
    AI_AUTOPATROL, /* 直线巡逻，不含转弯（可能也不需要单独写个模式） */
    AI_TURN,
    AI_FIRE_COMMAND,
  } AI_DATA;
  /* 这个变量如何跟notice建立联系 */
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

  Protocol_DownPackage_t from_host_{}; /* 从ai拿到的原始数据数组 */

  uint8_t ai_data_status_;

  uint8_t fire_command_;

  struct {
    RefereePckage ref{};
    MCUPckage mcu{};
  } to_host_;

  RefForAI ref_{};

  System::Thread thread_;

  System::Semaphore data_ready_;

  Message::Event event_; /* 为了上面那个功能，尝试中 */

  Message::Topic<Component::CMD::Data> cmd_tp_;

  Component::CMD::Data cmd_{};

  Component::Type::Quaternion quat_{};
  Device::Referee::Data raw_ref_{};
};
}  // namespace Device
