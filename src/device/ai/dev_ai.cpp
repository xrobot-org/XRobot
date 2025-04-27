#include "dev_ai.hpp"

#include "bsp_time.h"
#include "bsp_uart.h"
#include "comp_crc16.hpp"
#include "comp_crc8.hpp"

#define AI_CMD_LIMIT (0.08f)
#define AI_CTRL_SENSE (1.0f / 90.0f)
#define AI_LEN_RX_BUFF (sizeof(Protocol_DownPackage_t))
#define AI_LEN_TX_BUFF (sizeof(Protocol_UpPackage_t))

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
      // quat_sub.DumpData(ai->quat_);
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
  this->to_host_.crc16 = Component::CRC16::Calculate(
      reinterpret_cast<const uint8_t *>(&(this->to_host_)),
      sizeof(this->to_host_) - sizeof(uint16_t), CRC16_INIT);
  size_t len = sizeof(this->to_host_);
  void *src = NULL;
  src = &(to_host_);
  this->ref_updated_ = false;

  memcpy(txbuf, src, len);
  return bsp_uart_transmit(BSP_UART_AI, txbuf, len, false) == BSP_OK;
}

bool AI::Offline() {
  /* 离线移交控制权 */
  if (bsp_time_get_ms() - this->last_online_time_ > 200) {
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
  float temp = 0.0;

  this->to_host_.id = AI_ID_MCU;

  memcpy(&(this->to_host_.data.eulr), &(this->eulr_), sizeof(this->eulr_));
  memcpy(&(this->to_host_.data.yaw), &temp, sizeof(this->to_host_.data.yaw));
  memcpy(&(to_host_.data.pit), &temp, sizeof(this->to_host_.data.pit));
  memcpy(&(to_host_.data.rol), &temp, sizeof(this->to_host_.data.rol));

  this->to_host_.data.notice = this->notice_for_ai_;

  return true;
}

bool AI::PackRef() {
  this->to_host_.data.ball_speed = static_cast<float>(this->ref_.ball_speed);
  this->to_host_.data.rfid = this->ref_.rfid;
  this->to_host_.data.sentry_hp = this->ref_.hp;
  this->to_host_.data.game_progress = this->ref_.game_progress;
  this->to_host_.data.ballet_remain = this->ref_.bullet_num;
  this->ref_updated_ = true;
  this->to_host_.data.hero_x = this->ref_.hero_x;
  this->to_host_.data.hero_y = this->ref_.hero_y;
  this->to_host_.data.standard_3_x = this->ref_.infantry_3_x;
  this->to_host_.data.standard_3_y = this->ref_.infantry_3_y;
  this->to_host_.data.standard_4_x = this->ref_.infantry_4_x;
  this->to_host_.data.standard_4_y = this->ref_.infantry_4_y;
  this->to_host_.data.engineer_x = this->ref_.engineer_x;
  this->to_host_.data.engineer_y = this->ref_.engineer_y;
  return true;
}

void AI::DecideAction() {
  memcpy(&(this->cmd_.gimbal.eulr), &(this->from_host_.data.gimbal),
         sizeof(this->cmd_.gimbal.eulr));
  this->notice_ = this->from_host_.data.notice;

  if (this->cmd_.gimbal.eulr.yaw != this->last_auto_aim_eulr_.yaw ||
      this->cmd_.gimbal.eulr.pit != this->last_auto_aim_eulr_.pit) {
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

  navigation_enable_ = true;
  /* 裁判系统行为：确认复活、购买发弹量 */
  if (this->ref_.hp == 0) {
    this->action_.ai_to_referee = CONFIRM_RESURRECTION;
  } else if (this->ref_.bullet_num == 0 && this->ref_.coin_num > 200) {
    this->action_.ai_to_referee = EXCHANGE_BULLETS;
  } else {
    this->action_.ai_to_referee = NOTHING;
  }

  /* 底盘行为*/
  if (navigation_enable_ && damage_.is_damaged_ == false) {
    this->action_.ai_chassis = AI::Action::START_AUTO_CONTROL;
  } else if (damage_.is_damaged_ == true) {
    this->action_.ai_chassis = AI::Action::STOP_AUTO_CONTROL;
  }

  /* 云台行为 */
  if (this->notice_ == 2 || bsp_time_get_ms() - aim_time_ < 1300) {
    /* 自瞄/丢失目标1000ms内，进行视觉暂留 */
    this->action_.ai_gimbal = AI::Action::AUTO_AIM;
    this->cmd_.gimbal.mode = Component::CMD::GIMBAL_ABSOLUTE_CTRL;
  } else if (this->notice_ == 5) {
    this->cmd_.gimbal.mode = Component::CMD::GIMBAL_ABSOLUTE_CTRL;
    this->action_.ai_gimbal = AI::Action::SCANF;
  }

  /* 发射机构行为 */
  if (this->notice_ == 2) { /* 开火 */
    this->action_.ai_launcher = AI::Action::FIRE;
  } else if (this->notice_ == 5) {
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
        case START_AUTO_CONTROL:
          memcpy(&(this->cmd_.chassis),
                 &(this->from_host_.data.chassis_move_vec),
                 sizeof(this->from_host_.data.chassis_move_vec));
          break;
        case STOP_AUTO_CONTROL:
          this->event_.Active(AI_ROTOR);
          break;
      }

      switch (this->action_.ai_gimbal) {
        case AUTO_AIM:
          memcpy(&(this->cmd_.gimbal.eulr), &(this->from_host_.data.gimbal),
                 sizeof(this->cmd_.gimbal.eulr));
          if (cmd_.gimbal.eulr.pit == 0 && cmd_.gimbal.eulr.yaw == 0) {
            cmd_.gimbal.eulr.pit = eulr_.pit;
            cmd_.gimbal.eulr.yaw = eulr_.yaw;
          }
          break;
        case SCANF:
          /*yaw轴旋转加入随机数扰动*/
          static float smoothed_random =
              0.0f;            // 静态变量，用于存储平滑后的随机值
          float alpha = 0.1f;  // 平滑系数，控制新随机值的影响程度
          float raw_random =
              static_cast<float>(rand() % 10 - 5) / 1000.0f;  // 原始随机扰动
          smoothed_random = alpha * raw_random +
                            (1.0f - alpha) * smoothed_random;  // 低通滤波
          float yaw_rate = this->scanf_mode_.scanf_yaw_rate;
          this->target_scan_angle_ += yaw_rate * (1.0f + smoothed_random);
          // this->cmd_.gimbal.eulr.yaw = this->target_scan_angle_;
          this->cmd_.gimbal.eulr.yaw = this->eulr_.yaw;

          float t = static_cast<float>(bsp_time_get_ms()) / 1000.0f;
          // float pit_phase =
          //     fmodf(t * this->scanf_mode_.scanf_pit_omega, 2.0f * M_PI);
          // // 三角波输出范围 [0, 1]
          // float triangle_wave =
          //     (pit_phase < M_PI ? pit_phase / M_PI
          //                       : (2.0f * M_PI - pit_phase) / M_PI);
          // float mapped_triangle_wave = 2.0f * triangle_wave - 1.0f;
          // // 计算俯仰角
          // this->cmd_.gimbal.eulr.pit =
          //     this->scanf_mode_.scanf_pit_center +
          //     this->scanf_mode_.scanf_pit_range * mapped_triangle_wave;

          /*计算三角波*/
          float phase =
              fmodf(t * this->scanf_mode_.scanf_pit_omega, 2.0f * M_PI);
          float triangle_wave_mapped =
              (fabsf(phase / M_PI - 1.0f) * 2.0f) - 1.0f;
          this->cmd_.gimbal.eulr.pit =
              this->scanf_mode_.scanf_pit_center +
              this->scanf_mode_.scanf_pit_range * triangle_wave_mapped;
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

      this->cmd_tp_.Publish(this->cmd_);
      this->cmd_tp_.Publish(this->cmd_);
      this->cmd_tp_.Publish(this->cmd_);
    }
    /* OP控制模式，用于鼠标右键自瞄 */
    if (Component::CMD::GetCtrlMode() == Component::CMD::CMD_OP_CTRL) {
      memcpy(&(this->cmd_.gimbal.eulr), &(this->from_host_.data.gimbal),
             sizeof(this->cmd_.gimbal.eulr));

      if (cmd_.gimbal.eulr.pit == 0 && cmd_.gimbal.eulr.yaw == 0) {
        cmd_.gimbal.eulr.pit = eulr_.pit;
        cmd_.gimbal.eulr.yaw = eulr_.yaw;
      }

      memcpy(&(this->cmd_.chassis), &(this->from_host_.data.chassis_move_vec),
             sizeof(this->from_host_.data.chassis_move_vec));

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
    this->ref_.hp = this->raw_ref_.game_robot_hp.red_7;
    // this->ref_.hp = 400;

  } else {
    this->ref_.base_hp = this->raw_ref_.game_robot_hp.blue_base;
    this->ref_.outpost_hp = this->raw_ref_.game_robot_hp.blue_outpose;
    this->ref_.hp = this->raw_ref_.game_robot_hp.blue_7;
  }
  this->ref_.coin_num = this->raw_ref_.bullet_remain.coin_remain;
  this->ref_.bullet_num = this->raw_ref_.bullet_remain.bullet_17_remain;
  this->ref_.hero_x = this->raw_ref_.robot_pos_for_snetry.hero_x;
  this->ref_.hero_y = this->raw_ref_.robot_pos_for_snetry.hero_y;
  this->ref_.infantry_3_x = this->raw_ref_.robot_pos_for_snetry.standard_3_x;
  this->ref_.infantry_3_y = this->raw_ref_.robot_pos_for_snetry.standard_3_y;
  this->ref_.infantry_4_x = this->raw_ref_.robot_pos_for_snetry.standard_4_x;
  this->ref_.infantry_4_y = this->raw_ref_.robot_pos_for_snetry.standard_4_y;
  this->ref_.engineer_x = this->raw_ref_.robot_pos_for_snetry.engineer_x;
  this->ref_.engineer_y = this->raw_ref_.robot_pos_for_snetry.engineer_y;
  this->ref_.pos_angle = this->raw_ref_.robot_pos.angle;

  this->ref_.target_pos_x = this->raw_ref_.client_map.position_x;
  this->ref_.target_pos_y = this->raw_ref_.client_map.position_y;

  if (this->raw_ref_.robot_damage.damage_type == 0) {
    this->ref_.damaged_armor_id = this->raw_ref_.robot_damage.armor_id;
  }

  std::memcpy(&this->ref_.rfid, &this->raw_ref_.rfid, sizeof(uint32_t));
}
