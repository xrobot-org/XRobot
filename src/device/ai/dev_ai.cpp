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

AI::AI()
    : data_ready_(false),
      event_(Message::Event::FindEvent("cmd_event")),
      cmd_tp_("cmd_ai") {
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

    while (1) {
      /* 接收指令 */
      ai->StartRecv();

      if (ai->data_ready_.Wait(0)) {
        ai->PraseHost();
      } else {
        ai->Offline();
      }

      /* 发布控制命令 */
      ai->PackCMD();

      /* 发送数据到上位机 */
      quat_sub.DumpData(ai->quat_);
      ai->PackMCU();

      if (ref_sub.DumpData(ai->raw_ref_)) {
        ai->PraseRef();
        ai->PackRef();
      }

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
    this->cmd_.online = false;
    this->cmd_.ctrl_source = Component::CMD::CTRL_SOURCE_AI;
    this->cmd_tp_.Publish(this->cmd_);
  }
  return true;
}

bool AI::PackMCU() {
  this->to_host_.mcu.id = AI_ID_MCU;
  memcpy(&(this->to_host_.mcu.package.data.quat), &(this->quat_),
         sizeof(this->quat_));
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

bool AI::PackCMD() {
  memcpy(&(this->cmd_.gimbal.eulr), &(this->from_host_.data.gimbal),
         sizeof(this->cmd_.gimbal.eulr));

  memcpy(&(this->cmd_.ext.extern_channel),
         &(this->from_host_.data.extern_channel),
         sizeof(this->cmd_.ext.extern_channel));

  memcpy(&(this->cmd_.chassis), &(this->from_host_.data.chassis_move_vec),
         sizeof(this->from_host_.data.chassis_move_vec));

  this->notice_ = this->from_host_.data.notice;

  memcpy(&(this->last_eulr_), &(this->cmd_.gimbal.eulr),
         sizeof(this->cmd_.gimbal.eulr));

  this->cmd_.ctrl_source = Component::CMD::CTRL_SOURCE_AI;

  this->cmd_tp_.Publish(this->cmd_);

  if (!Component::CMD::Online()) {
    return false;
  }

  /* 控制权在AI */
  if (Component::CMD::GetCtrlSource() == Component::CMD::CTRL_SOURCE_AI) {
    if (this->cmd_.online) {
      if (this->notice_ == AI_NOTICE_AUTO_AIM) {
        this->cmd_.gimbal.mode = Component::CMD::GIMBAL_ABSOLUTE_CTRL;
        this->event_.Active(AI_FIND_TARGET);
      } else if (this->notice_ == AI_NOTICE_FIRE) {
        this->cmd_.gimbal.mode = Component::CMD::GIMBAL_ABSOLUTE_CTRL;
        this->event_.Active(AI_FIND_TARGET);
        this->event_.Active(AI_FIRE_COMMAND);
      } else {
        this->event_.Active(AI_AUTOPATROL);
      }
    } else {
      this->event_.Active(AI_OFFLINE);
    }
  }

  this->notice_ = 0;

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
}
