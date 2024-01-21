/*
  裁判系统抽象。
*/

#include "dev_referee.hpp"

#include "bsp_time.h"
#include "bsp_uart.h"
#include "comp_crc16.hpp"
#include "comp_crc8.hpp"

#define REF_HEADER_SOF (0xA5)
#define REF_LEN_RX_BUFF (0xFF)
#define REF_LEN_TX_BUFF (0xFF)

#define REF_UI_BOX_UP_OFFSET (4)
#define REF_UI_BOX_BOT_OFFSET (-14)

#define REF_UI_RIGHT_START_W (0.85f)

#define REF_UI_MODE_LINE1_H (0.7f)
#define REF_UI_MODE_LINE2_H (0.68f)
#define REF_UI_MODE_LINE3_H (0.66f)
#define REF_UI_MODE_LINE4_H (0.64f)

#define REF_UI_MODE_OFFSET_1_LEFT (-6)
#define REF_UI_MODE_OFFSET_1_RIGHT (44)
#define REF_UI_MODE_OFFSET_2_LEFT (54)
#define REF_UI_MODE_OFFSET_2_RIGHT (102)
#define REF_UI_MODE_OFFSET_3_LEFT (114)
#define REF_UI_MODE_OFFSET_3_RIGHT (162)
#define REF_UI_MODE_OFFSET_4_LEFT (174)
#define REF_UI_MODE_OFFSET_4_RIGHT (222)

using namespace Device;

static uint8_t rxbuf[REF_LEN_RX_BUFF];

Referee::UIPack Referee::ui_pack_;
Referee *Referee::self_;

Referee::Referee() : event_(Message::Event::FindEvent("cmd_event")) {
  self_ = this;

  auto rx_cplt_callback = [](void *arg) {
    Referee *ref = static_cast<Referee *>(arg);
    ref->raw_ready_.Post();
  };

  auto tx_cplt_callback = [](void *arg) {
    Referee *ref = static_cast<Referee *>(arg);
    ref->packet_sent_.Post();
  };

  auto idle_line_callback = [](void *arg) {
    static_cast<void>(arg);
    bsp_uart_abort_receive(BSP_UART_REF);
  };

  auto abort_rx_cplt_callback = [](void *arg) {
    Referee *ref = static_cast<Referee *>(arg);
    ref->raw_ready_.Post();
  };

  bsp_uart_register_callback(BSP_UART_REF, BSP_UART_RX_CPLT_CB,
                             rx_cplt_callback, this);
  bsp_uart_register_callback(BSP_UART_REF, BSP_UART_ABORT_RX_CPLT_CB,
                             abort_rx_cplt_callback, this);
  bsp_uart_register_callback(BSP_UART_REF, BSP_UART_IDLE_LINE_CB,
                             idle_line_callback, this);
  bsp_uart_register_callback(BSP_UART_REF, BSP_UART_TX_CPLT_CB,
                             tx_cplt_callback, this);
#if !UI_MODE_NONE

#endif

  auto ref_recv_thread = [](Referee *ref) {
    while (1) {
      ref->StartRecv();

#if REF_FORCE_ONLINE
      ref->raw_ready_.Wait(100);
      ref->Prase();
#else
      if (!ref->raw_ready_.Wait(100)) { /* 判断裁判系统数据是否接收完成 */
        ref->Offline(); /* 长时间未接收到数据，裁判系统离线 */
      } else {
        ref->Prase(); /* 解析裁判系统数据 */
      }
#endif

      /* 发布裁判系统数据 */
      ref->ref_data_tp_.Publish(ref->ref_data_);
    }
  };

  this->recv_thread_.Create(ref_recv_thread, this, "ref_recv_thread",
                            DEVICE_REF_RECV_TASK_STACK_DEPTH,
                            System::Thread::REALTIME);
  auto ref_trans_thread = [](Referee *ref) {
    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      ref->UpdateUI();
      ref->trans_thread_.SleepUntil(40, last_online_time);
    }
  };
  this->trans_thread_.Create(ref_trans_thread, this, "ref_trans_thread",
                             DEVICE_REF_TRANS_TASK_STACK_DEPTH,
                             System::Thread::MEDIUM);
}

void Referee::Offline() { this->ref_data_.status = OFFLINE; }

bool Referee::StartRecv() {
  return bsp_uart_receive(BSP_UART_REF, rxbuf, REF_LEN_RX_BUFF, false) ==
         BSP_OK;
}

void Referee::Prase() {
  this->ref_data_.status = RUNNING;
  size_t data_length = bsp_uart_get_count(BSP_UART_REF);

  const uint8_t *index = rxbuf; /* const 保护原始rxbuf不被修改 */
  const uint8_t *const RXBUF_END = rxbuf + data_length;

  while (index < RXBUF_END) {
    /* 1.处理帧头 */
    /* 1.1遍历所有找到SOF */
    while ((*index != REF_HEADER_SOF) && (index < RXBUF_END)) {
      index++;
    }
    /* 1.2将剩余数据当做帧头部 */
    const Referee::Header *header =
        reinterpret_cast<const Referee::Header *>(index);

    /* 1.3验证完整性 */
    if (!Component::CRC8::Verify(reinterpret_cast<const uint8_t *>(header),
                                 sizeof(*header))) {
      index++;
      continue;
    }
    index += sizeof(*header);

    /* 2.处理CMD ID */
    /* 2.1将剩余数据当做CMD ID处理 */
    const Referee::CMDID *cmd_id =
        reinterpret_cast<const Referee::CMDID *>(index);
    index += sizeof(*cmd_id);

    /* 3.处理数据段 */
    const void *source = index;
    void *destination = NULL;
    size_t size = 0;

    switch (static_cast<int>(*cmd_id)) {
      case REF_CMD_ID_GAME_STATUS:
        destination = &(this->ref_data_.game_status);
        size = sizeof(this->ref_data_.game_status);
        break;
      case REF_CMD_ID_GAME_RESULT:
        destination = &(this->ref_data_.game_result);
        size = sizeof(this->ref_data_.game_result);
        break;
      case REF_CMD_ID_GAME_ROBOT_HP:
        destination = &(this->ref_data_.game_robot_hp);
        size = sizeof(this->ref_data_.game_robot_hp);
        break;
      case REF_CMD_ID_FIELD_EVENTS:
        destination = &(this->ref_data_.field_event);
        size = sizeof(this->ref_data_.field_event);
        break;
      case REF_CMD_ID_SUPPLY_ACTION:
        destination = &(this->ref_data_.supply_action);
        size = sizeof(this->ref_data_.supply_action);
        break;
      case REF_CMD_ID_WARNING:
        destination = &(this->ref_data_.warning);
        size = sizeof(this->ref_data_.warning);
        break;
      case REF_CMD_ID_DART_COUNTDOWN:
        destination = &(this->ref_data_.dart_countdown);
        size = sizeof(this->ref_data_.dart_countdown);
        break;
      case REF_CMD_ID_ROBOT_STATUS:
        destination = &(this->ref_data_.robot_status);
        size = sizeof(this->ref_data_.robot_status);
        break;
      case REF_CMD_ID_POWER_HEAT_DATA:
        destination = &(this->ref_data_.power_heat);
        size = sizeof(this->ref_data_.power_heat);
        break;
      case REF_CMD_ID_ROBOT_POS:
        destination = &(this->ref_data_.robot_pos);
        size = sizeof(this->ref_data_.robot_pos);
        break;
      case REF_CMD_ID_ROBOT_BUFF:
        destination = &(this->ref_data_.robot_buff);
        size = sizeof(this->ref_data_.robot_buff);
        break;
      case REF_CMD_ID_DRONE_ENERGY:
        destination = &(this->ref_data_.drone_energy);
        size = sizeof(this->ref_data_.drone_energy);
        break;
      case REF_CMD_ID_ROBOT_DMG:
        destination = &(this->ref_data_.robot_damage);
        size = sizeof(this->ref_data_.robot_damage);
        break;
      case REF_CMD_ID_LAUNCHER_DATA:
        destination = &(this->ref_data_.launcher_data);
        size = sizeof(this->ref_data_.launcher_data);
        break;
      case REF_CMD_ID_BULLET_REMAINING:
        destination = &(this->ref_data_.bullet_remain);
        size = sizeof(this->ref_data_.bullet_remain);
        break;
      case REF_CMD_ID_RFID:
        destination = &(this->ref_data_.rfid);
        size = sizeof(this->ref_data_.rfid);
        break;
      case REF_CMD_ID_DART_CLIENT:
        destination = &(this->ref_data_.dart_client);
        size = sizeof(this->ref_data_.dart_client);
        break;
      case REF_CMD_ID_ROBOT_POS_TO_SENTRY:
        destination = &(this->ref_data_.robot_pos_for_snetry);
        size = sizeof(this->ref_data_.robot_pos_for_snetry);
        break;
      case REF_CMD_ID_RADAR_MARK:
        destination = &(this->ref_data_.radar_mark_progress);
        size = sizeof(this->ref_data_.radar_mark_progress);
        break;
      case REF_CMD_ID_SENTRY_DECISION:
        destination = &(this->ref_data_.sentry_decision);
        size = sizeof(this->ref_data_.sentry_decision);
        break;
      case REF_CMD_ID_RADAR_DECISION:
        destination = &(this->ref_data_.radar_decision);
        size = sizeof(this->ref_data_.radar_decision);
        break;
      case REF_CMD_ID_INTER_STUDENT:
        destination = &(this->ref_data_.robot_ineraction_data);
        size = sizeof(this->ref_data_.robot_ineraction_data);
        break;
      case REF_CMD_ID_INTER_STUDENT_CUSTOM:
        destination = &(this->ref_data_.custom_controller);
        size = sizeof(this->ref_data_.custom_controller);
        break;
      case REF_CMD_ID_CLIENT_MAP:
        destination = &(this->ref_data_.client_map);
        size = sizeof(this->ref_data_.client_map);
        break;
      case REF_CMD_ID_KEYBOARD_MOUSE:
        destination = &(this->ref_data_.keyboard_mouse);
        size = sizeof(this->ref_data_.keyboard_mouse);
        break;
      case REF_CMD_ID_CUSTOM_KEYBOARD_MOUSE:
        destination = &(this->ref_data_.custom_key_mouse_data);
        size = sizeof(this->ref_data_.custom_key_mouse_data);
        break;
      case REF_CMD_ID_SENTRY_POS_DATA:
        destination = &(this->ref_data_.sentry_postion);
        size = sizeof(this->ref_data_.sentry_postion);
        break;
      case REF_CMD_ID_ROBOT_POS_DATA:
        destination = &(this->ref_data_.robot_position);
        size = sizeof(this->ref_data_.robot_position);
        break;

      default:
        return;
    }
    index += size;

    /* 4.处理帧尾 */
    index += sizeof(Referee::Tail);

    /* 验证无误则接受数据 */
    if (Component::CRC16::Verify(
            reinterpret_cast<const uint8_t *>((header)),
            static_cast<uint8_t>(
                index - reinterpret_cast<const uint8_t *>((header))))) {
      memcpy(destination, source, size);
    }
    if (ref_data_.robot_damage.damage_type == 0x0 &&
        !last_data_.robot_damage.damage_type) {
      this->event_.Active(REF_ATTACKED);
    }
    if (ref_data_.game_status.game_progress == 4 &&
        !last_data_.game_status.game_progress) {
      this->event_.Active(REF_GAME_START);
    }
    memcpy(&(this->last_data_), &(destination), sizeof(Data));
  }
#if REF_VIRTUAL
#if REF_FORCE_ONLINE
  this->ref_data_.status = RUNNING;
#endif
  this->ref_data_.power_heat.launcher_id1_17_heat = REF_HEAT_LIMIT_17;
  this->ref_data_.power_heat.launcher_42_heat = REF_HEAT_LIMIT_42;
  this->ref_data_.robot_status.launcher_id1_17_speed_limit = REF_LAUNCH_SPEED;
  this->ref_data_.robot_status.launcher_42_speed_limit = REF_LAUNCH_SPEED;
  this->ref_data_.robot_status.chassis_power_limit = REF_POWER_LIMIT;
  this->ref_data_.power_heat.chassis_pwr_buff = REF_POWER_BUFF;
#endif
}

bool Referee::UpdateUI() {
  this->packet_sent_.Wait(UINT32_MAX);

  this->ui_lock_.Wait(UINT32_MAX);

  bool done = false;
  uint32_t ele_counter = 0;
  uint32_t pack_size = 0;
  CMDID cmd_id = REF_STDNT_CMD_ID_UI_DEL;
  Component::UI::GraphicOperation op = Component::UI::UI_GRAPHIC_OP_NOTHING;

  if (!done && this->static_del_data_.Size() > 0) {
    cmd_id = REF_STDNT_CMD_ID_UI_DEL;
    pack_size = sizeof(UIDelPack);
    done = true;
    op = Component::UI::UI_GRAPHIC_OP_ADD;
  }

  if (bsp_time_get_ms() % 6000 > 3000) {
    if (!done && this->static_string_data_.Size() > 0) {
      cmd_id = REF_STDNT_CMD_ID_UI_STR;
      pack_size = sizeof(UIStringPack);
      done = true;
      op = Component::UI::UI_GRAPHIC_OP_ADD;
    }

    static_ele_data_.Reset();
  } else {
    if (!done && this->static_ele_data_.Size() > 0) {
      switch (this->static_ele_data_.Size()) {
        case 0:
          break;
        case 1:
          cmd_id = REF_STDNT_CMD_ID_UI_DRAW1;
          done = true;
          op = Component::UI::UI_GRAPHIC_OP_ADD;
          ele_counter = 1;
          pack_size = sizeof(UIElePack_1);
          break;
        case 2:
        case 3:
        case 4:
          cmd_id = REF_STDNT_CMD_ID_UI_DRAW2;
          done = true;
          op = Component::UI::UI_GRAPHIC_OP_ADD;
          ele_counter = 2;
          pack_size = sizeof(UIElePack_2);
          break;
        case 5:
        case 6:
          cmd_id = REF_STDNT_CMD_ID_UI_DRAW5;
          done = true;
          op = Component::UI::UI_GRAPHIC_OP_ADD;
          ele_counter = 5;
          pack_size = sizeof(UIElePack_5);
          break;
        default:
          cmd_id = REF_STDNT_CMD_ID_UI_DRAW7;
          done = true;
          op = Component::UI::UI_GRAPHIC_OP_ADD;
          ele_counter = 7;
          pack_size = sizeof(UIElePack_7);
          break;
      }
    }

    static_string_data_.Reset();
  }

  if (!done && this->del_data_.Size() > 0) {
    cmd_id = REF_STDNT_CMD_ID_UI_DEL;
    pack_size = sizeof(UIDelPack);
    done = true;
    op = Component::UI::UI_GRAPHIC_OP_REWRITE;
  }

  if (!done && this->string_data_.Size() > 0) {
    cmd_id = REF_STDNT_CMD_ID_UI_STR;
    pack_size = sizeof(UIStringPack);
    done = true;
    op = Component::UI::UI_GRAPHIC_OP_REWRITE;
  }

  if (!done && this->ele_data_.Size() > 0) {
    switch (this->ele_data_.Size()) {
      case 0:
        break;
      case 1:
        cmd_id = REF_STDNT_CMD_ID_UI_DRAW1;
        done = true;
        op = Component::UI::UI_GRAPHIC_OP_REWRITE;
        ele_counter = 1;
        pack_size = sizeof(UIElePack_1);
        break;
      case 2:
      case 3:
      case 4:
        cmd_id = REF_STDNT_CMD_ID_UI_DRAW2;
        done = true;
        op = Component::UI::UI_GRAPHIC_OP_REWRITE;
        ele_counter = 2;
        pack_size = sizeof(UIElePack_2);
        break;
      case 5:
      case 6:
        cmd_id = REF_STDNT_CMD_ID_UI_DRAW5;
        done = true;
        op = Component::UI::UI_GRAPHIC_OP_REWRITE;
        ele_counter = 5;
        pack_size = sizeof(UIElePack_5);
        break;
      default:
        cmd_id = REF_STDNT_CMD_ID_UI_DRAW7;
        done = true;
        ele_counter = 7;
        op = Component::UI::UI_GRAPHIC_OP_REWRITE;
        pack_size = sizeof(UIElePack_7);
        break;
    }
  }

  if (!done) {
    this->ui_lock_.Post();
    this->packet_sent_.Post();
    return false;
  }

  this->ui_pack_.raw.cmd_id = REF_CMD_ID_INTER_STUDENT;

  SetUIHeader(this->ui_pack_.raw.student_header, cmd_id,
              static_cast<RobotID>(this->ref_data_.robot_status.robot_id));

  SetPacketHeader(this->ui_pack_.raw.frame_header, pack_size - 9);

  if (ele_counter) {
    for (uint32_t i = 0; i < ele_counter; i++) {
      if (op == Component::UI::UI_GRAPHIC_OP_REWRITE) {
        this->ele_data_.Receive(this->ui_pack_.ele_7.ele_data[i]);
      } else {
        this->static_ele_data_.Receive(this->ui_pack_.ele_7.ele_data[i]);
      }
    }

    uint16_t *crc_addr = reinterpret_cast<uint16_t *>(
        &this->ui_pack_.ele_7.ele_data[ele_counter]);

    *crc_addr = Component::CRC16::Calculate(
        reinterpret_cast<const uint8_t *>(&this->ui_pack_),
        pack_size - sizeof(uint16_t), CRC16_INIT);
  } else if (cmd_id == REF_STDNT_CMD_ID_UI_DEL) {
    if (op == Component::UI::UI_GRAPHIC_OP_REWRITE) {
      this->del_data_.Receive(this->ui_pack_.del.del_data);
    } else {
      this->static_del_data_.Receive(this->ui_pack_.del.del_data);
    }
    this->ui_pack_.del.crc16 = Component::CRC16::Calculate(
        reinterpret_cast<const uint8_t *>(&this->ui_pack_),
        pack_size - sizeof(uint16_t), CRC16_INIT);

  } else if (cmd_id == REF_STDNT_CMD_ID_UI_STR) {
    if (op == Component::UI::UI_GRAPHIC_OP_REWRITE) {
      this->string_data_.Receive(this->ui_pack_.str.str_data);
    } else {
      this->static_string_data_.Receive(this->ui_pack_.str.str_data);
    }
    this->ui_pack_.str.crc16 = Component::CRC16::Calculate(
        reinterpret_cast<const uint8_t *>(&this->ui_pack_),
        pack_size - sizeof(uint16_t), CRC16_INIT);
  }

  bsp_uart_transmit(BSP_UART_REF, reinterpret_cast<uint8_t *>(&this->ui_pack_),
                    pack_size, false);

  this->ui_lock_.Post();

  return true;
}

bool Referee::AddUI(Component::UI::Ele ui_data) {
  self_->ui_lock_.Wait(UINT32_MAX);
  if (ui_data.op == Component::UI::UI_GRAPHIC_OP_ADD) {
    self_->static_ele_data_.Send(ui_data);
  } else {
    self_->ele_data_.Send(ui_data);
  }
  self_->ui_lock_.Post();

  return true;
}

bool Referee::AddUI(Component::UI::Del ui_data) {
  self_->ui_lock_.Wait(UINT32_MAX);
  self_->del_data_.Send(ui_data);
  self_->ui_lock_.Post();

  return true;
}

bool Referee::AddUI(Component::UI::Str ui_data) {
  self_->ui_lock_.Wait(UINT32_MAX);
  if (ui_data.graphic.op == Component::UI::UI_GRAPHIC_OP_ADD) {
    self_->static_string_data_.Send(ui_data);
  } else {
    self_->string_data_.Send(ui_data);
  }
  self_->ui_lock_.Post();

  return true;
}

void Referee::SetUIHeader(Referee::InterStudentHeader &header,
                          const Referee::CMDID CMD_ID,
                          Referee::RobotID robot_id) {
  header.cmd_id = CMD_ID;
  header.id_sender = robot_id;
  if (robot_id > 100) {
    header.id_receiver = robot_id - 101 + 0x0165;
  } else {
    header.id_receiver = robot_id + 0x0100;
  }
}

void Referee::SetPacketHeader(Referee::Header &header, uint16_t data_length) {
  static uint8_t seq = 0;
  header.sof = REF_HEADER_SOF;
  header.data_length = data_length;
  header.seq = seq++;
  header.crc8 = Component::CRC8::Calculate(
      reinterpret_cast<const uint8_t *>(&header),
      sizeof(Referee::Header) - sizeof(uint8_t), CRC8_INIT);
}
