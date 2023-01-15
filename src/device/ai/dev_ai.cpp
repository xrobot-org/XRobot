#include "dev_ai.hpp"

#include "bsp_delay.h"
#include "bsp_time.h"
#include "bsp_uart.h"
#include "comp_crc16.hpp"
#include "comp_crc8.hpp"
#include "comp_utils.hpp"

#define AI_CMD_LIMIT (0.08f)
#define AI_CTRL_SENSE (1.0f / 90.0f)
#define AI_LEN_RX_BUFF (sizeof(Protocol_DownPackage_t))
#define AI_LEN_TX_BUFF \
  (sizeof(Protocol_UpPackageMCU_t) + sizeof(Protocol_UpPackageReferee_t))

static uint8_t rxbuf[AI_LEN_RX_BUFF];
static uint8_t txbuf[AI_LEN_TX_BUFF];

using namespace Device;

AI::AI() : data_ready_(false), cmd_tp_("cmd_ai") {
  auto rx_cplt_callback = [](void *arg) {
    AI *ai = static_cast<AI *>(arg);
    ai->data_ready_.GiveFromISR();
  };

  bsp_uart_register_callback(BSP_UART_AI, BSP_UART_RX_CPLT_CB, rx_cplt_callback,
                             this);

  Component::CMD::RegisterController(this->cmd_tp_);

  auto ai_thread = [](AI *ai) {
    auto quat_sub = Message::Subscriber("gimbal_quat", ai->quat_);
    Message::Subscriber ref_sub("referee", ai->raw_ref_);

    while (1) {
      /* 接收指令 */
      ai->StartRecv();

      if (ai->data_ready_.Take(0)) {
        ai->PraseHost();
      } else {
        ai->Offline();
      }

      /* 发布控制命令 */
      ai->PackCMD();

      /* 发送数据到上位机 */
      quat_sub.DumpData();
      ai->PackMCU();

      if (ref_sub.DumpData()) {
        ai->PraseRef();
        ai->PackRef();
      }

      ai->StartTrans();

      System::Thread::Sleep(2);
    }
  };

  this->thread_.Create(ai_thread, this, "ai_thread", 256,
                       System::Thread::Realtime);
}

bool AI::StartRecv() {
  return bsp_uart_receive(BSP_UART_AI, (uint8_t *)&(rxbuf), sizeof(rxbuf),
                          false) == HAL_OK;
}

bool AI::PraseHost() {
  if (Component::CRC16::Verify((const uint8_t *)&(rxbuf),
                               sizeof(this->form_host))) {
    this->cmd_.online = true;
    this->last_online_time_ = bsp_time_get();
    memcpy(&(this->form_host), rxbuf, sizeof(this->form_host));
    memset(rxbuf, 0, AI_LEN_RX_BUFF);
    return true;
  }
  return false;
}

bool AI::StartTrans() {
  size_t len = sizeof(this->to_host.mcu_);
  void *src;
  if (this->ref_updated_) {
    len += sizeof(this->to_host.ref_);
    src = &(this->to_host);
  } else {
    src = &(this->to_host.mcu_);
  }
  this->ref_updated_ = false;

  memcpy(txbuf, src, len);
  return bsp_uart_transmit(BSP_UART_AI, txbuf, len, false) == BSP_OK;
}

bool AI::Offline() {
  /* 离线移交控制权 */
  if (bsp_time_get() - this->last_online_time_ > 0.05f) {
    this->cmd_.online = false;
    this->cmd_tp_.Publish(this->cmd_);
  }
  return true;
}

bool AI::PackMCU() {
  this->to_host.mcu_.id = AI_ID_MCU;
  memcpy(&(this->to_host.mcu_.package.data.quat), &(this->quat_),
         sizeof(this->quat_));
  this->to_host.mcu_.package.crc16 = Component::CRC16::Calculate(
      (const uint8_t *)&(this->to_host.mcu_.package),
      sizeof(this->to_host.mcu_.package) - sizeof(uint16_t), CRC16_INIT);
  return true;
}

bool AI::PackRef() {
  this->to_host.ref_.id = AI_ID_REF;
  this->to_host.mcu_.package.data.ball_speed = this->ref_.ball_speed;
  this->to_host.ref_.package.data.arm = this->ref_.robot_id;
  this->to_host.ref_.package.data.rfid = this->ref_.robot_buff;
  this->to_host.ref_.package.data.team = this->ref_.team;
  this->to_host.ref_.package.data.race = this->ref_.game_type;
  this->to_host.ref_.package.crc16 = Component::CRC16::Calculate(
      (const uint8_t *)&(this->to_host.ref_.package),
      sizeof(this->to_host.ref_.package) - sizeof(uint16_t), CRC16_INIT);

  this->ref_updated_ = true;

  return true;
}

bool AI::PackCMD() {
  this->cmd_.gimbal.mode = Component::CMD::GimbalAbsoluteCtrl;

  memcpy(&(this->cmd_.gimbal.eulr), &(this->form_host.data.gimbal),
         sizeof(this->cmd_.gimbal.eulr));

  this->cmd_.ctrl_source = Component::CMD::ControlSourceAI;

  this->cmd_tp_.Publish(this->cmd_);

  return true;
}

void AI::PraseRef() {
#if RB_HERO
  this->ref_.ball_speed = this->ref_.data_.robot_status.launcher_42_speed_limit;
#else
  this->ref_.ball_speed =
      this->raw_ref_.robot_status.launcher_id1_17_speed_limit;
#endif

  this->ref_.max_hp = this->raw_ref_.robot_status.max_hp;

  this->ref_.hp = this->raw_ref_.robot_status.remain_hp;

  if (this->raw_ref_.robot_status.robot_id < Referee::REF_BOT_BLU_HERO)
    this->ref_.team = AI_TEAM_RED;
  else
    this->ref_.team = AI_TEAM_BLUE;

  this->ref_.status = this->raw_ref_.status;

  if (this->raw_ref_.rfid.high_ground == 1)
    this->ref_.robot_buff |= AI_RFID_SNIP;

  else if (this->raw_ref_.rfid.energy_mech == 1)
    this->ref_.robot_buff |= AI_RFID_BUFF;

  else
    this->ref_.robot_buff = 0;

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
