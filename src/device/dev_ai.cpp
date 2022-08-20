#include "dev_ai.hpp"

#include <string.h>

#include "bsp_delay.h"
#include "bsp_uart.h"
#include "comp_crc16.hpp"
#include "comp_crc8.hpp"
#include "comp_utils.hpp"
#include "om.h"

#define AI_CMD_LIMIT (0.08f)
#define AI_CTRL_SENSE (1.0f / 90.0f)
#define AI_LEN_RX_BUFF (sizeof(Protocol_DownPackage_t))
#define AI_LEN_TX_BUFF \
  (sizeof(Protocol_UpPackageMCU_t) + sizeof(Protocol_UpPackageReferee_t))

static uint8_t rxbuf[AI_LEN_RX_BUFF];
static uint8_t txbuf[AI_LEN_TX_BUFF];

using namespace Device;

AI::AI() {
  auto rx_cplt_callback = [](void *arg) {
    AI *ai = (AI *)arg;
    ai->sem.data_ready_.GiveFromISR();
  };

  bsp_uart_register_callback(BSP_UART_AI, BSP_UART_RX_CPLT_CB, rx_cplt_callback,
                             this);

  auto ai_thread = [](void *arg) {
    AI *ai = (AI *)arg;

    Referee::Referee::Data ref_raw;

    DECLARE_TOPIC(host_tp, ai->cmd_, "cmd_host", true);

    DECLARE_SUBER(eulr_sub, ai->eulr_, "gimbal_eulr");
    DECLARE_SUBER(quat_sub, ai->quat_, "gimbal_quat");
    DECLARE_SUBER(ref_sub, ref_raw, "referee");

    while (1) {
      /* 接收指令 */
      ai->StartRecv();

      if (ai->sem.data_ready_.Take(0)) {
        ai->PraseHost();
      } else {
        ai->Offline();
      }

      /* AI在线,发布控制命令 */
      if (ai->online_) {
        eulr_sub.DumpData();
        ai->PackCMD();
        host_tp.Publish();
      }

      /* 发送数据到上位机 */
      quat_sub.DumpData();
      ai->PackMCU();

      if (ref_sub.DumpData()) {
        ai->PraseRef(&ref_raw);
        ai->PackRef();
      }

      ai->StartTrans();

      System::Thread::Sleep(2);
    }
  };

  THREAD_DECLEAR(this->thread_, ai_thread, 256, System::Thread::Realtime, this);
}

bool AI::StartRecv() {
  return bsp_uart_receive(BSP_UART_AI, (uint8_t *)&(rxbuf), sizeof(rxbuf),
                          false) == HAL_OK;
}

bool AI::PraseHost() {
  if (Component::CRC16::Verify((const uint8_t *)&(rxbuf),
                               sizeof(this->form_host))) {
    this->online_ = true;
    this->last_online_time_ = System::Thread::GetTick();
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
  if (System::Thread::GetTick() - this->last_online_time_ > 50) {
    this->online_ = false;
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
  this->cmd_.gimbal_delta.yaw =
      (this->form_host.data.gimbal.yaw - this->eulr_.yaw) * AI_CTRL_SENSE;
  this->cmd_.gimbal_delta.pit =
      (this->form_host.data.gimbal.pit - this->eulr_.pit) * AI_CTRL_SENSE;
  clampf(&this->cmd_.gimbal_delta.yaw, -AI_CMD_LIMIT, AI_CMD_LIMIT);
  clampf(&this->cmd_.gimbal_delta.pit, -AI_CMD_LIMIT, AI_CMD_LIMIT);

  this->cmd_.fire = (this->form_host.data.notice & AI_NOTICE_FIRE);
  this->cmd_.chassis_move_vec.vx = this->form_host.data.chassis_move_vec.vx;
  this->cmd_.chassis_move_vec.vy = this->form_host.data.chassis_move_vec.vy;
  this->cmd_.chassis_move_vec.wz = this->form_host.data.chassis_move_vec.wz;

  return true;
}

void AI::PraseRef(Device::Referee::Data *ref) {
#if RB_HERO
  this->ref_.ball_speed = ref->robot_status.launcher_42_speed_limit;
#else
  this->ref_.ball_speed = ref->robot_status.launcher_id1_17_speed_limit;
#endif

  this->ref_.max_hp = ref->robot_status.max_hp;

  this->ref_.hp = ref->robot_status.remain_hp;

  if (ref->robot_status.robot_id < Referee::REF_BOT_BLU_HERO)
    this->ref_.team = AI_TEAM_RED;
  else
    this->ref_.team = AI_TEAM_BLUE;

  this->ref_.status = ref->status;

  if (ref->rfid.high_ground == 1)
    this->ref_.robot_buff |= AI_RFID_SNIP;

  else if (ref->rfid.energy_mech == 1)
    this->ref_.robot_buff |= AI_RFID_BUFF;

  else
    this->ref_.robot_buff = 0;

  switch (ref->game_status.game_type) {
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

  switch (ref->robot_status.robot_id % 100) {
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
