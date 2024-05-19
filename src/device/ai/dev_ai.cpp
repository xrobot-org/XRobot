#include "dev_ai.hpp"

#include "bsp_time.h"
#include "bsp_uart.h"
#include "comp_crc16.hpp"
#include "comp_crc8.hpp"

#define AI_CMD_LIMIT (0.08f)
#define AI_CTRL_SENSE (1.0f / 90.0f)
#define AI_LEN_RX_BUFF (sizeof(Protocol_DownPackage_t))
#define AI_LEN_TX_BUFF \
  (sizeof(Protocol_UpPackageMCU_t) + sizeof(Protocol_UpPackageReferee_t))

static uint8_t rxbuf[AI_LEN_RX_BUFF];
static uint8_t txbuf[AI_LEN_TX_BUFF];

using namespace Device;

AI::AI(bool autoscan_enable)
    : autoscan_enable_(autoscan_enable),
      event_(Message::Event::FindEvent("cmd_event")),
      cmd_tp_("cmd_ai"),
      data_ready_(false) {
  auto rx_cplt_callback = [](void *arg) {
    AI *ai = static_cast<AI *>(arg);
    ai->data_ready_.Post();
  };

  bsp_uart_register_callback(BSP_UART_AI, BSP_UART_RX_CPLT_CB, rx_cplt_callback,
                             this);

  Component::CMD::RegisterController(this->cmd_tp_);

  auto ai_thread = [](AI *ai) {
    auto quat_sub =
        Message::Subscriber<Component::Type::Quaternion>("imu_quat");
    auto ref_sub = Message::Subscriber<Device::Referee::Data>("referee");
    auto yaw_sub = Message::Subscriber<float>("chassis_yaw");

    auto eulr_sub = Message::Subscriber<Component::Type::Eulr>("imu_eulr");

    while (1) {
      /* 接收云台数据 */
      yaw_sub.DumpData(ai->chassis_yaw_offset_);
      eulr_sub.DumpData(ai->eulr_); /* imu */

      /* 接收裁判系统数据 */
      if (ref_sub.DumpData(ai->raw_ref_)) {
        ai->PraseRef();
        ai->PackRef();
      }
      /* 接收上位机数据 */
      ai->StartRecv();
      if (ai->data_ready_.Wait(0)) {
        ai->PraseHost();
      } else {
        ai->Offline();
      }

      /* 决策与命令发布 */
      ai->DecideAction();
      ai->PackCMD();
      ai->ai_tp_.Publish(ai->cmd_for_ref_);

      /* 发送数据到上位机 */
      quat_sub.DumpData(ai->quat_);
      ai->PackMCU();

      ai->StartTrans();

      System::Thread::Sleep(2);
    }
  };

  this->thread_.Create(ai_thread, this, "ai_thread", DEVICE_AI_TASK_STACK_DEPTH,
                       System::Thread::REALTIME);
}

bool AI::StartRecv() {
  return bsp_uart_receive(BSP_UART_AI, rxbuf, sizeof(rxbuf), false) == BSP_OK;
}

bool AI::PraseHost() {
  if (Component::CRC16::Verify(rxbuf, sizeof(this->from_host_))) {
    this->cmd_.online = true;
    this->last_online_time_ = bsp_time_get_ms();
    memcpy(&(this->from_host_), rxbuf, sizeof(this->from_host_));
    memset(rxbuf, 0, AI_LEN_RX_BUFF);
    return true;
  }
  return false;
}

bool AI::StartTrans() {
  size_t len = sizeof(this->to_host_.mcu);
  void *src = NULL;
  if (this->ref_updated_) {
    len += sizeof(this->to_host_.ref);
    src = &(this->to_host_);
  } else {
    src = &(this->to_host_.mcu);
  }
  this->ref_updated_ = false;

  memcpy(txbuf, src, len);
  return bsp_uart_transmit(BSP_UART_AI, txbuf, len, false) == BSP_OK;
}

bool AI::Offline() {
  /* 离线移交控制权 */
  if (bsp_time_get_ms() - this->last_online_time_ > 200) {
    memset(&this->cmd_, 0, sizeof(cmd_));
    if (!this->autoscan_enable_) {
      this->cmd_.online = true;
    } else {
      this->cmd_.online = false;
    }
    this->cmd_.ctrl_source = Component::CMD::CTRL_SOURCE_AI;
    this->cmd_tp_.Publish(this->cmd_);
  }
  return true;
}

bool AI::PackMCU() {
  this->to_host_.mcu.id = AI_ID_MCU;
  memcpy(&(this->to_host_.mcu.package.data.quat), &(this->quat_),
         sizeof(this->quat_));
  this->to_host_.mcu.package.data.notice = this->notice_for_ai_;
  this->to_host_.mcu.package.crc16 = Component::CRC16::Calculate(
      reinterpret_cast<const uint8_t *>(&(this->to_host_.mcu.package)),
      sizeof(this->to_host_.mcu.package) - sizeof(uint16_t), CRC16_INIT);
  return true;
}

bool AI::PackRef() {
  this->to_host_.ref.id = AI_ID_REF;
  this->to_host_.mcu.package.data.ball_speed =
      static_cast<float>(this->ref_.ball_speed);
  this->to_host_.ref.package.data.arm = this->ref_.robot_id;
  this->to_host_.ref.package.data.rfid = this->ref_.robot_buff;
  this->to_host_.ref.package.data.team = this->ref_.team;
  this->to_host_.ref.package.data.race = this->ref_.game_type;
  this->to_host_.ref.package.crc16 = Component::CRC16::Calculate(
      reinterpret_cast<const uint8_t *>(&(this->to_host_.ref.package)),
      sizeof(this->to_host_.ref.package) - sizeof(uint16_t), CRC16_INIT);

  this->ref_updated_ = true;

  return true;
}

void AI::DecideAction() {
  memcpy(&(this->cmd_.gimbal.eulr), &(this->from_host_.data.gimbal),
         sizeof(this->cmd_.gimbal.eulr));
  this->notice_ = this->from_host_.data.notice;
  /* AI自瞄数据不更新，重置notice_ */
  // TODO:检测更新方式不合理
  if (this->cmd_.gimbal.eulr.yaw == this->last_auto_aim_eulr_.yaw ||
      this->cmd_.gimbal.eulr.pit == this->last_auto_aim_eulr_.pit) {
    this->notice_ = 0;
  } else {
    this->aim_time_ = bsp_time_get_ms(); /* 自瞄锁定时刻 */
  }
  memcpy(&(this->last_auto_aim_eulr_), &(this->cmd_.gimbal.eulr),
         sizeof(this->cmd_.gimbal.eulr));

  /* 判定是否受击打，一定时间内第一次受击打方向优先级最高 */
  if (this->raw_ref_.robot_damage.damage_type == 0x0 &&
      (this->raw_ref_.robot_damage.damage_type != damage_.type_) &&
      (damage_.is_damaged_ == false)) {
    this->damage_.is_damaged_ = true;
    this->damage_.id_ = this->raw_ref_.robot_damage.armor_id;
    this->damage_.yaw_offset_ = this->chassis_yaw_offset_;
    this->damage_.gimbal_yaw_ = this->eulr_.yaw;
    this->damage_.time_ = bsp_time_get_ms();
  }
  if (bsp_time_get_ms() - this->damage_.time_ > 1500) {
    this->damage_.is_damaged_ = false;
  }

  /* 判定是否可以开始导航 */
  if (this->from_host_.data.chassis_move_vec.vx == 0 && damage_.is_damaged_) {
    navigation_enable_ = false;
  } else {
    navigation_enable_ = true;
  }

  /* 裁判系统行为：确认复活、购买发弹量 */
  if (this->ref_.hp == 0) {
    this->action_.ai_to_referee = CONFIRM_RESURRECTION;
  } else if (this->ref_.bullet_num == 0 && this->ref_.coin_num > 200) {
    this->action_.ai_to_referee = EXCHANGE_BULLETS;
  } else {
    this->action_.ai_to_referee = NOTHING;
  }

  /* 底盘行为*/
  if (navigation_enable_) {
    if (this->ref_.outpost_hp > 200) { /* 认为哨兵&基地无敌 */
      this->action_.ai_chassis = AI::Action::TO_HIGHWAY;
      this->notice_for_ai_ = AI::Action::TO_HIGHWAY;
    } else {
      if (this->ref_.hp < 100 ||
          (this->action_.ai_to_referee == EXCHANGE_BULLETS)) { /* max = 400 */
        this->action_.ai_chassis = AI::Action::TO_SUPPLY;
        this->notice_for_ai_ = AI::Action::TO_SUPPLY;
      } else {
        this->action_.ai_chassis = AI::Action::TO_PATROL_AREA;
        this->notice_for_ai_ = AI::Action::TO_PATROL_AREA;
      }
    }
  } else {
    this->action_.ai_chassis = AI::Action::ROTOR;
  }

  /* 云台行为 */
  if (this->notice_ == AI_NOTICE_AUTO_AIM || this->notice_ == AI_NOTICE_FIRE ||
      bsp_time_get_ms() - this->aim_time_ < 1000) {
    /* 自瞄/丢失目标1000ms内，进行视觉暂留 */
    this->action_.ai_gimbal = AI::Action::AUTO_AIM;
    this->cmd_.gimbal.mode = Component::CMD::GIMBAL_ABSOLUTE_CTRL;
  } else if (damage_.is_damaged_) {
    this->cmd_.gimbal.mode = Component::CMD::GIMBAL_ABSOLUTE_CTRL;
    this->action_.ai_gimbal = AI::Action::AFFECTED;
  } else {
    this->cmd_.gimbal.mode = Component::CMD::GIMBAL_ABSOLUTE_CTRL;
    this->action_.ai_gimbal = AI::Action::SCANF;
  }

  /* 发射机构行为 */
  if (this->notice_ == AI_NOTICE_FIRE) { /* 开火 */
    this->action_.ai_launcher = AI::Action::FIRE;
  } else {
    this->action_.ai_launcher = AI::Action::CEASEFIRE; /* 不发弹 */
  }
}

bool AI::PackCMD() {
  /* 确保遥控器开关最高控制权，关遥控器即断控 */
  if (!Component::CMD::Online()) {
    return false;
  }

  /* 控制源：AI */
  if (Component::CMD::GetCtrlSource() == Component::CMD::CTRL_SOURCE_AI) {
    /* AUTO控制模式，用于全自动机器人 */
    if (Component::CMD::GetCtrlMode() == Component::CMD::CMD_AUTO_CTRL) {
      switch (this->action_.ai_chassis) {
        case TO_PATROL_AREA:
        case TO_HIGHWAY:
        case TO_OUTPOST:
        case TO_SUPPLY:
          memcpy(&(this->cmd_.chassis),
                 &(this->from_host_.data.chassis_move_vec),
                 sizeof(this->from_host_.data.chassis_move_vec));
          break;
        case ROTOR:
          this->event_.Active(AI_ROTOR);
          break;
      }

      switch (this->action_.ai_gimbal) {
        case AUTO_AIM:
          memcpy(&(this->cmd_.gimbal.eulr), &(this->from_host_.data.gimbal),
                 sizeof(this->cmd_.gimbal.eulr));
          break;
        case AFFECTED:
          this->cmd_.gimbal.eulr.yaw = this->damage_.gimbal_yaw_ +
                                       this->damage_.id_ * (M_2PI / 4.0) -
                                       this->damage_.yaw_offset_;
          this->cmd_.gimbal.eulr.pit = 0.0f;
          this->gimbal_scan_start_angle_ =
              this->eulr_.yaw;            /* 更新yaw扫描的起始位置 */
          this->target_scan_angle_ = 0.0; /* 置零yaw扫描的位置增量 */
          break;
        case SCANF:
          this->target_scan_angle_ += this->scanf_mode_.scanf_yaw_rate;
          this->cmd_.gimbal.eulr.yaw =
              this->target_scan_angle_ + this->gimbal_scan_start_angle_;
          this->cmd_.gimbal.eulr.pit =
              this->scanf_mode_.scanf_pit_center +
              this->scanf_mode_.scanf_pit_range *
                  sinf(this->scanf_mode_.scanf_pit_omega *
                       static_cast<float>(bsp_time_get_ms()) / 1000.0f);
          break;
      }

      switch (this->action_.ai_launcher) {
        case FIRE:
          this->event_.Active(AI_FIRE_COMMAND); /* 发弹指令，采用连发模式 */
          break;
        case CEASEFIRE:
          this->event_.Active(AI_STOP_FIRE);
          break;
      }

      switch (this->action_.ai_to_referee) {
        case CONFIRM_RESURRECTION:
          cmd_for_ref_.confirm_resurrection = 1;
          cmd_for_ref_.buy_resurrection = 0;
          cmd_for_ref_.buy_bullet_num = 0;
          cmd_for_ref_.remote_buy_bullet_times = 0;
          cmd_for_ref_.romote_buy_hp_times = 0;
          break;
        case EXCHANGE_BULLETS:
          cmd_for_ref_.confirm_resurrection = 0;
          cmd_for_ref_.buy_resurrection = 0;
          cmd_for_ref_.buy_bullet_num = this->last_buy_bullet_num_ + 50;
          cmd_for_ref_.remote_buy_bullet_times = 0;
          cmd_for_ref_.romote_buy_hp_times = 0;
          break;
        case NOTHING:
          cmd_for_ref_.confirm_resurrection = 0;
          cmd_for_ref_.buy_resurrection = 0;
          cmd_for_ref_.buy_bullet_num = this->last_buy_bullet_num_;
          cmd_for_ref_.remote_buy_bullet_times = 0;
          cmd_for_ref_.romote_buy_hp_times = 0;
          break;
      }
      this->last_buy_bullet_num_ = cmd_for_ref_.buy_bullet_num;

      /* 比赛开始前不运行 */
      if (this->ref_.game_progress == GAMING ||
          this->ref_.game_progress == PREPARATION ||
          this->ref_.game_progress == WAITTING) {
        this->cmd_tp_.Publish(this->cmd_);
      }
    }
    /* OP控制模式，用于鼠标右键自瞄 */
    else if (Component::CMD::GetCtrlMode() == Component::CMD::CMD_OP_CTRL) {
      memcpy(&(this->cmd_.gimbal.eulr), &(this->from_host_.data.gimbal),
             sizeof(this->cmd_.gimbal.eulr));

      memcpy(&(this->cmd_.ext.extern_channel),
             &(this->from_host_.data.extern_channel),
             sizeof(this->cmd_.ext.extern_channel));

      memcpy(&(this->cmd_.chassis), &(this->from_host_.data.chassis_move_vec),
             sizeof(this->from_host_.data.chassis_move_vec));

      memcpy(&(this->cmd_.ext.extern_channel),
             &(this->from_host_.data.extern_channel),
             sizeof(this->cmd_.ext.extern_channel));

      this->notice_ = this->from_host_.data.notice;

      this->cmd_.ctrl_source = Component::CMD::CTRL_SOURCE_AI;

      this->cmd_tp_.Publish(this->cmd_);
      this->cmd_tp_.Publish(this->cmd_);
      this->cmd_tp_.Publish(this->cmd_);
    }
  }
  return true;
}

void AI::PraseRef() {
#if RB_HERO
  this->ref_.ball_speed = BULLET_SPEED_LIMIT_42MM
#else
  this->ref_.ball_speed = BULLET_SPEED_LIMIT_17MM;
#endif

                          this->ref_.max_hp =
      this->raw_ref_.robot_status.max_hp;

  this->ref_.hp = this->raw_ref_.robot_status.remain_hp;

  if (this->raw_ref_.robot_status.robot_id < Referee::REF_BOT_BLU_HERO) {
    this->ref_.team = AI_TEAM_RED;
  } else {
    this->ref_.team = AI_TEAM_BLUE;
  }
  this->ref_.status = this->raw_ref_.status;

  if (this->raw_ref_.rfid.own_highland_annular == 1 ||
      this->raw_ref_.rfid.enemy_highland_annular == 1) {
    this->ref_.robot_buff |= AI_RFID_SNIP;
  } else if (this->raw_ref_.rfid.own_energy_mech_activation == 1) {
    this->ref_.robot_buff |= AI_RFID_BUFF;
  } else {
    this->ref_.robot_buff = 0;
  }
  switch (this->raw_ref_.game_status.game_type) {
    case Referee::REF_GAME_TYPE_RMUC:
      this->ref_.game_type = AI_RACE_RMUC;
      break;
    case Referee::REF_GAME_TYPE_RMUT:
      this->ref_.game_type = AI_RACE_RMUT;
      break;
    case Referee::REF_GAME_TYPE_RMUL_3V3:
      this->ref_.game_type = AI_RACE_RMUL3;
      break;
    case Referee::REF_GAME_TYPE_RMUL_1V1:
      this->ref_.game_type = AI_RACE_RMUL1;
      break;
    default:
      return;
  }

  switch (this->raw_ref_.robot_status.robot_id % 100) {
    case Referee::REF_BOT_RED_HERO:
      this->ref_.robot_id = AI_ARM_HERO;
      break;
    case Referee::REF_BOT_RED_ENGINEER:
      this->ref_.robot_id = AI_ARM_ENGINEER;
      break;
    case Referee::REF_BOT_RED_DRONE:
      this->ref_.robot_id = AI_ARM_DRONE;
      break;
    case Referee::REF_BOT_RED_SENTRY:
      this->ref_.robot_id = AI_ARM_SENTRY;
      break;
    case Referee::REF_BOT_RED_RADER:
      this->ref_.robot_id = AI_ARM_RADAR;
      break;
    default:
      this->ref_.robot_id = AI_ARM_INFANTRY;
  }

  this->ref_.game_progress = this->raw_ref_.game_status.game_progress;

  if (this->raw_ref_.robot_status.robot_id < 100) {
    this->ref_.base_hp = this->raw_ref_.game_robot_hp.red_base;
    this->ref_.outpost_hp = this->raw_ref_.game_robot_hp.red_outpose;
    this->ref_.own_virtual_shield_value =
        this->raw_ref_.field_event.virtual_shield_value;
  } else {
    this->ref_.base_hp = this->raw_ref_.game_robot_hp.blue_base;
    this->ref_.outpost_hp = this->raw_ref_.game_robot_hp.blue_outpose;
    this->ref_.own_virtual_shield_value =
        this->raw_ref_.field_event.virtual_shield_value;
  }
  this->ref_.coin_num = this->raw_ref_.bullet_remain.coin_remain;
  this->ref_.pos_x = this->raw_ref_.robot_pos.x;
  this->ref_.pos_y = this->raw_ref_.robot_pos.y;
  this->ref_.pos_angle = this->raw_ref_.robot_pos.angle;

  this->ref_.target_pos_x = this->raw_ref_.client_map.position_x;
  this->ref_.target_pos_y = this->raw_ref_.client_map.position_y;

  if (this->raw_ref_.robot_damage.damage_type == 0) {
    this->ref_.damaged_armor_id = this->raw_ref_.robot_damage.armor_id;
  }
}
