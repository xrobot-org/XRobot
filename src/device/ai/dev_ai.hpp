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

  typedef struct {
    float scanf_yaw_rate;
    float scanf_pit_center;
    float scanf_pit_range;
    float scanf_pit_omega;
  } ScanfMode;

  typedef enum {
    AI_OFFLINE = 127,
    AI_INDENPENDENT,
    AI_FOLLOW,
    AI_ROTOR,
    AI_FIRE_COMMAND,
    AI_STOP_FIRE,
  } AIControlData;

  typedef enum {
    WAITTING = 0,
    PREPARATION = 1,
    REF_INSPECTION = 2,
    COUNTDOWN_5S = 3,
    GAMING = 4,
    GAME_END = 5,
  } GameProgress;
  typedef enum {
    TO_PATROL_AREA = 0, /* 哨兵巡逻区 */
    TO_SUPPLY = 1,      /* 补给区 */
    TO_HIGHWAY = 2,     /* 公路区 */
    TO_OUTPOST = 3,     /* 前哨站 */
    ROTOR = 4,

    SCANF = 5,
    AUTO_AIM = 6,
    AFFECTED = 7, /* 反击*/

    CEASEFIRE = 8, /* 停火 */
    FIRE = 9,

    NOTHING = 10,
    CONFIRM_RESURRECTION = 11, /* 确认复活 */
    EXCHANGE_BULLETS = 12,     /* 兑换弹丸 */
    /* 其他行为暂不考虑 */
  } Action;

  typedef struct __attribute__((packed)) {
    uint32_t confirm_resurrection : 1;
    uint32_t buy_resurrection : 1;
    uint32_t buy_bullet_num : 11;
    uint32_t remote_buy_bullet_times : 4;
    uint32_t romote_buy_hp_times : 4;
    uint32_t res : 11;
  } SentryDecisionData;
  typedef struct {
    uint8_t ai_chassis;    /* 小陀螺、导航 */
    uint8_t ai_gimbal;     /* 扫描、自瞄、反击 */
    uint8_t ai_launcher;   /* 不发弹丸、开火 */
    uint8_t ai_to_referee; /* 确认复活、购买发弹量、购买血量 */
  } AICtrlAction;

  AI(bool autoscan_enable = false);

  bool StartRecv();

  bool PraseHost();

  bool StartTrans();

  bool Offline();

  bool PackMCU();

  bool PackRef();

  void PraseRef();

  bool PackCMD();

  void DecideAction();

 private:
  /* function */
  bool autoscan_enable_ = true; /* AI自动扫描，不启用则默认AI全程在线 */

  /* run status */
  bool navigation_enable_ = 0;
  bool ref_updated_ = false;
  uint8_t last_buy_bullet_num_ = 0;

  /* time record */
  uint32_t last_online_time_ = 0;
  uint32_t aim_time_;

  /* angle record */
  float chassis_yaw_offset_ = 0;
  float gimbal_scan_start_angle_;
  Component::Type::Eulr eulr_;
  Component::Type::Quaternion quat_;
  Component::Type::CycleValue target_scan_angle_ = 0.0;
  struct {
    float yaw; /* 偏航角（Yaw angle） */
    float pit; /* 俯仰角（Pitch angle） */
    float rol; /* 翻滚角（Roll angle） */
  } last_auto_aim_eulr_;

  /* notice */
  uint8_t notice_;
  uint8_t notice_for_ai_;

  /* Damage Status */
  struct {
    uint8_t type_;
    bool is_damaged_;
    uint8_t id_;
    uint32_t time_;
    Component::Type::CycleValue yaw_offset_;
    Component::Type::CycleValue gimbal_yaw_;
  } damage_;

  /* Data */
  Protocol_DownPackage_t from_host_{};

  struct {
    RefereePckage ref{};
    MCUPckage mcu{};
  } to_host_;

  RefForAI ref_;

  Component::CMD::Data cmd_{};

  AICtrlAction action_;

  ScanfMode scanf_mode_ = {
      .scanf_yaw_rate = 0.0025f,
      .scanf_pit_center = 0.04f,
      .scanf_pit_range = 0.22f,
      .scanf_pit_omega = 5.0f,
  };

  SentryDecisionData cmd_for_ref_;

  /* Topic & Event */

  Message::Event event_;

  Message::Topic<Component::CMD::Data> cmd_tp_;

  Message::Topic<SentryDecisionData> ai_tp_ =
      Message::Topic<SentryDecisionData>("sentry_cmd_for_ref");

  Device::Referee::Data raw_ref_{};

  /* Task Control */
  System::Thread thread_;

  System::Semaphore data_ready_;
};
}  // namespace Device
