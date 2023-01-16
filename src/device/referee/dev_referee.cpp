/*
  裁判系统抽象。
*/

#include "dev_referee.hpp"

#include "bsp_delay.h"
#include "bsp_uart.h"
#include "comp_crc16.hpp"
#include "comp_crc8.hpp"
#include "protocol.h"

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
static uint8_t txbuf[REF_LEN_TX_BUFF];

System::Semaphore *ui_fast_refresh;
System::Semaphore *ui_slow_refresh;

Referee::Referee() {
  ui_fast_refresh = &(this->ui_fast_refresh_);
  ui_slow_refresh = &(this->ui_slow_refresh_);

  auto rx_cplt_callback = [](void *arg) {
    Referee *ref = static_cast<Referee *>(arg);
    ref->raw_ready_.GiveFromISR();
  };

  auto tx_cplt_callback = [](void *arg) {
    Referee *ref = static_cast<Referee *>(arg);
    ref->packet_sent_.GiveFromISR();
  };

  auto idle_line_callback = [](void *arg) {
    RM_UNUSED(arg);
    bsp_uart_abort_receive(BSP_UART_REF);
  };

  auto abort_rx_cplt_callback = [](void *arg) {
    Referee *ref = static_cast<Referee *>(arg);
    ref->raw_ready_.GiveFromISR();
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
      ref->raw_ready_.Take(100);
      ref->Prase();
#else
      if (!ref->raw_ready_.Take(100)) { /* 判断裁判系统数据是否接收完成 */
        ref->Offline(); /* 长时间未接收到数据，裁判系统离线 */
      } else {
        ref->Prase(); /* 解析裁判系统数据 */
      }
#endif

      /* 发布裁判系统数据 */
      ref->ref_data_tp_.Publish(ref->ref_data_);
    }
  };

  this->recv_thread_.Create(ref_recv_thread, this, "ref_recv_thread", 256,
                            System::Thread::Realtime);
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
      case REF_CMD_ID_DART_STATUS:
        destination = &(this->ref_data_.dart_status);
        size = sizeof(this->ref_data_.dart_status);
        break;
      case REF_CMD_ID_ICRA_ZONE_STATUS:
        destination = &(this->ref_data_.icra_zone);
        size = sizeof(this->ref_data_.icra_zone);
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
        destination = &(this->ref_data_.robot_danage);
        size = sizeof(this->ref_data_.robot_danage);
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
      case REF_CMD_ID_CLIENT_MAP:
        destination = &(this->ref_data_.client_map);
        size = sizeof(this->ref_data_.client_map);
        break;
      case REF_CMD_ID_KEYBOARD_MOUSE:
        destination = &(this->ref_data_.keyboard_mouse);
        size = sizeof(this->ref_data_.keyboard_mouse);
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

bool Referee::RefreshUI() {
#if UI_MODE_OP
  ui_ele_t ele;
  ui_string_t string;

  const float kW = this->ui_.ui.screen.width;
  const float kH = this->ui_.ui.screen.height;

  float box_pos_left = 0.0f, box_pos_right = 0.0f;

  static ui_graphic_op_t graphic_op = UI_GRAPHIC_OP_ADD;

  /* UI静态元素刷新 */
  if (this->ui_slow_refresh_.Take(0)) {
    graphic_op = UI_GRAPHIC_OP_ADD;
    this->ui_.ui.refresh_fsm = 0;

    ui_draw_string(&string, "8", graphic_op, UI_GRAPHIC_LAYER_CONST, UI_GREEN,
                   UI_DEFAULT_WIDTH * 10, 80, UI_CHAR_DEFAULT_WIDTH,
                   (uint16_t)(kW * REF_UI_RIGHT_START_W),
                   (uint16_t)(kH * REF_UI_MODE_LINE1_H),
                   "CHAS  FLLW  FL35  ROTR");
    ui_stash_string(&(this->ui_.ui), &string);

    ui_draw_string(&string, "9", graphic_op, UI_GRAPHIC_LAYER_CONST, UI_GREEN,
                   UI_DEFAULT_WIDTH * 10, 80, UI_CHAR_DEFAULT_WIDTH,
                   (uint16_t)(kW * REF_UI_RIGHT_START_W),
                   (uint16_t)(kH * REF_UI_MODE_LINE2_H),
                   "GMBL  RELX  ABSL  RLTV");
    ui_stash_string(&(this->ui_.ui), &string);

    ui_draw_string(&string, "a", graphic_op, UI_GRAPHIC_LAYER_CONST, UI_GREEN,
                   UI_DEFAULT_WIDTH * 10, 80, UI_CHAR_DEFAULT_WIDTH,
                   (uint16_t)(kW * REF_UI_RIGHT_START_W),
                   (uint16_t)(kH * REF_UI_MODE_LINE3_H),
                   "SHOT  RELX  SAFE  LOAD");
    ui_stash_string(&(this->ui_.ui), &string);

    ui_draw_string(&string, "b", graphic_op, UI_GRAPHIC_LAYER_CONST, UI_GREEN,
                   UI_DEFAULT_WIDTH * 10, 80, UI_CHAR_DEFAULT_WIDTH,
                   (uint16_t)(kW * REF_UI_RIGHT_START_W),
                   (uint16_t)(kH * REF_UI_MODE_LINE4_H),
                   "FIRE  SNGL  BRST  CONT");
    ui_stash_string(&(this->ui_.ui), &string);

    ui_draw_line(&ele, "c", graphic_op, UI_GRAPHIC_LAYER_CONST, UI_GREEN,
                 UI_DEFAULT_WIDTH * 3, (uint16_t)(kW * 0.4f),
                 (uint16_t)(kH * 0.2f), (uint16_t)(kW * 0.4f),
                 (uint16_t)(kH * 0.2f + 50.f));
    ui_stash_graphic(&(this->ui_.ui), &ele);

    ui_draw_string(&string, "d", graphic_op, UI_GRAPHIC_LAYER_CONST, UI_GREEN,
                   UI_DEFAULT_WIDTH * 10, 80, UI_CHAR_DEFAULT_WIDTH,
                   (uint16_t)(kW * REF_UI_RIGHT_START_W), (uint16_t)(kH * 0.4f),
                   "CTRL  JS  KM");
    ui_stash_string(&(this->ui_.ui), &string);

    ui_draw_string(&string, "e", graphic_op, UI_GRAPHIC_LAYER_CONST, UI_GREEN,
                   UI_DEFAULT_WIDTH * 20, 80, UI_CHAR_DEFAULT_WIDTH * 2,
                   (uint16_t)(kW * 0.6f - 26.0f), (uint16_t)(kH * 0.2f + 10.0f),
                   "CAP");
    ui_stash_string(&(this->ui_.ui), &string);

    return true;
  }

  /* UI动态元素刷新 */
  if (this->ui_fast_refresh_.Take(0)) {
    /* 使用状态机算法，每次更新一个图层 */
    switch (this->ui_.ui.refresh_fsm) {
      case 0: {
        this->ui_.ui.refresh_fsm++;

        /* 更新电容状态 */
        if (this->ui_.cap_ui.online) {
          ui_draw_arc(&ele, "3", graphic_op, UI_GRAPHIC_LAYER_CAP, UI_GREEN, 0,
                      (uint16_t)(this->ui_.cap_ui.percentage * 360.f),
                      UI_DEFAULT_WIDTH * 5, (uint16_t)(kW * 0.6f),
                      (uint16_t)(kH * 0.2f), 50, 50);
        } else {
          ui_draw_arc(&ele, "3", graphic_op, UI_GRAPHIC_LAYER_CAP, UI_YELLOW, 0,
                      360, UI_DEFAULT_WIDTH * 5, (uint16_t)(kW * 0.6f),
                      (uint16_t)(kH * 0.2), 50, 50);
        }
        ui_stash_graphic(&(this->ui_.ui), &ele);

        /* 更新云台模式选择框 */
        switch (this->ui_.gimbal_ui.mode) {
          case Component::CMD::Relax:
            box_pos_left = REF_UI_MODE_OFFSET_2_LEFT;
            box_pos_right = REF_UI_MODE_OFFSET_2_RIGHT;
            break;
          case Component::CMD::Absolute:
            box_pos_left = REF_UI_MODE_OFFSET_3_LEFT;
            box_pos_right = REF_UI_MODE_OFFSET_3_RIGHT;
            break;
          case Component::CMD::GIMBAL_MODE_RELATIVE:
            box_pos_left = REF_UI_MODE_OFFSET_4_LEFT;
            box_pos_right = REF_UI_MODE_OFFSET_4_RIGHT;
            break;
          default:
            box_pos_left = 0.0f;
            box_pos_right = 0.0f;
            break;
        }
        if (box_pos_left != 0.0f && box_pos_right != 0.0f) {
          ui_draw_rectangle(
              &ele, "4", graphic_op, UI_GRAPHIC_LAYER_GIMBAL, UI_GREEN,
              UI_DEFAULT_WIDTH,
              (uint16_t)(kW * REF_UI_RIGHT_START_W + box_pos_left),
              (uint16_t)(kH * REF_UI_MODE_LINE2_H + REF_UI_BOX_UP_OFFSET),
              (uint16_t)(kW * REF_UI_RIGHT_START_W + box_pos_right),
              (uint16_t)(kH * REF_UI_MODE_LINE2_H + REF_UI_BOX_BOT_OFFSET));
          ui_stash_graphic(&(this->ui_.ui), &ele);
        }

        /* 更新发射器模式选择框 */
        switch (this->ui_.launcher_ui.mode) {
          case Component::CMD::Relax:
            box_pos_left = REF_UI_MODE_OFFSET_2_LEFT;
            box_pos_right = REF_UI_MODE_OFFSET_2_RIGHT;
            break;
          case Component::CMD::Safe:
            box_pos_left = REF_UI_MODE_OFFSET_3_LEFT;
            box_pos_right = REF_UI_MODE_OFFSET_3_RIGHT;
            break;
          case Component::CMD::Loaded:
            box_pos_left = REF_UI_MODE_OFFSET_4_LEFT;
            box_pos_right = REF_UI_MODE_OFFSET_4_RIGHT;
            break;
          default:
            box_pos_left = 0.0f;
            box_pos_right = 0.0f;
            break;
        }
        if (box_pos_left != 0.0f && box_pos_right != 0.0f) {
          ui_draw_rectangle(
              &ele, "5", graphic_op, UI_GRAPHIC_LAYER_LAUNCHER, UI_GREEN,
              UI_DEFAULT_WIDTH,
              (uint16_t)(kW * REF_UI_RIGHT_START_W + box_pos_left),
              (uint16_t)(kH * REF_UI_MODE_LINE3_H + REF_UI_BOX_UP_OFFSET),
              (uint16_t)(kW * REF_UI_RIGHT_START_W + box_pos_right),
              (uint16_t)(kH * REF_UI_MODE_LINE3_H + REF_UI_BOX_BOT_OFFSET));
          ui_stash_graphic(&(this->ui_.ui), &ele);
        }

        /* 更新开火模式选择框 */
        switch (this->ui_.launcher_ui.fire) {
          case Component::CMD::Single:
            box_pos_left = REF_UI_MODE_OFFSET_2_LEFT;
            box_pos_right = REF_UI_MODE_OFFSET_2_RIGHT;
            break;
          case Component::CMD::Burst:
            box_pos_left = REF_UI_MODE_OFFSET_3_LEFT;
            box_pos_right = REF_UI_MODE_OFFSET_3_RIGHT;
            break;
          case Component::CMD::Continued:
            box_pos_left = REF_UI_MODE_OFFSET_4_LEFT;
            box_pos_right = REF_UI_MODE_OFFSET_4_RIGHT;
            break;
          default:
            box_pos_left = 0.0f;
            box_pos_right = 0.0f;
            break;
        }
        if (box_pos_left != 0.0f && box_pos_right != 0.0f) {
          ui_draw_rectangle(
              &ele, "6", graphic_op, UI_GRAPHIC_LAYER_LAUNCHER, UI_GREEN,
              UI_DEFAULT_WIDTH,
              (uint16_t)(kW * REF_UI_RIGHT_START_W + box_pos_left),
              (uint16_t)(kH * REF_UI_MODE_LINE4_H + REF_UI_BOX_UP_OFFSET),
              (uint16_t)(kW * REF_UI_RIGHT_START_W + box_pos_right),
              (uint16_t)(kH * REF_UI_MODE_LINE4_H + REF_UI_BOX_BOT_OFFSET));
          ui_stash_graphic(&(this->ui_.ui), &ele);
        }

        /* 更新控制权选择框 */
        switch (this->ui_.cmd_ui.ctrl_method) {
          case Component::CMD::CMD_METHOD_MOUSE_KEYBOARD:
            ui_draw_rectangle(&ele, "7", graphic_op, UI_GRAPHIC_LAYER_CMD,
                              UI_GREEN, UI_DEFAULT_WIDTH,
                              (uint16_t)(kW * REF_UI_RIGHT_START_W + 96.f),
                              (uint16_t)(kH * 0.4f + REF_UI_BOX_UP_OFFSET),
                              (uint16_t)(kW * REF_UI_RIGHT_START_W + 120.f),
                              (uint16_t)(kH * 0.4f + REF_UI_BOX_BOT_OFFSET));
            break;
          case Component::CMD::CMD_METHOD_JOYSTICK_SWITCH:
            ui_draw_rectangle(&ele, "7", graphic_op, UI_GRAPHIC_LAYER_CMD,
                              UI_GREEN, UI_DEFAULT_WIDTH,
                              (uint16_t)(kW * REF_UI_RIGHT_START_W + 56.f),
                              (uint16_t)(kH * 0.4f + REF_UI_BOX_UP_OFFSET),
                              (uint16_t)(kW * REF_UI_RIGHT_START_W + 80.f),
                              (uint16_t)(kH * 0.4f + REF_UI_BOX_BOT_OFFSET));
            break;
        }
        ui_stash_graphic(&(this->ui_.ui), &ele);
        break;
      }
      case 1: {
        this->ui_.ui.refresh_fsm++;

        /*更新拨弹电机状态*/
        float trig_start = this->ui_.launcher_ui.trig / M_2PI * 360.f;
        float trig_end = this->ui_.launcher_ui.trig / M_2PI * 360.f;
        circle_add(&trig_end, 60.0f, 360);
        if (trig_end >= 360.f) trig_end = 360.f;
        ui_draw_arc(&ele, "f", graphic_op, UI_GRAPHIC_LAYER_LAUNCHER, UI_GREEN,
                    (uint16_t)trig_start, (uint16_t)trig_end,
                    UI_DEFAULT_WIDTH * 5, (uint16_t)(kW * 0.4f),
                    (uint16_t)(kH * 0.1f), 50, 50);
        ui_stash_graphic(&(this->ui_.ui), &ele);

        /*更新摩擦轮电机状态*/
        if (this->ui_.launcher_ui.fric_percent[0] == 0 ||
            this->ui_.launcher_ui.fric_percent[1] == 0) {
          ui_draw_arc(&ele, "g", graphic_op, UI_GRAPHIC_LAYER_LAUNCHER,
                      UI_YELLOW, 0, 360, UI_DEFAULT_WIDTH * 5,
                      (uint16_t)(kW * 0.6f), (uint16_t)(kH * 0.1f), 50, 50);
        } else {
          ui_draw_arc(
              &ele, "g", graphic_op, UI_GRAPHIC_LAYER_LAUNCHER, UI_GREEN,
              (uint16_t)180 - 170 * this->ui_.launcher_ui.fric_percent[0],
              (uint16_t)(180 + 170 * this->ui_.launcher_ui.fric_percent[1]),
              UI_DEFAULT_WIDTH * 5, (uint16_t)(kW * 0.6f),
              (uint16_t)(kH * 0.1f), 50, 50);
        }
        ui_stash_graphic(&(this->ui_.ui), &ele);

        break;
      }

      case 2: {
        this->ui_.ui.refresh_fsm++;
        /* 更新云台底盘相对方位 */
        const float kLEN = 22;
        ui_draw_line(
            &ele, "1", graphic_op, UI_GRAPHIC_LAYER_CHASSIS, UI_GREEN,
            UI_DEFAULT_WIDTH * 12, (uint16_t)(kW * 0.4f), (uint16_t)(kH * 0.2f),
            (uint16_t)(kW * 0.4f + sinf(this->ui_.chassis_ui.angle) * 2 * kLEN),
            (uint16_t)(kH * 0.2f +
                       cosf(this->ui_.chassis_ui.angle) * 2 * kLEN));

        ui_stash_graphic(&(this->ui_.ui), &ele);

        /* 更新底盘模式选择框 */
        switch (this->ui_.chassis_ui.mode) {
          case Component::CMD::FollowGimbal:
            box_pos_left = REF_UI_MODE_OFFSET_2_LEFT;
            box_pos_right = REF_UI_MODE_OFFSET_2_RIGHT;
            break;
          case Component::CMD::CHASSIS_MODE_FOLLOW_GIMBAL_35:
            box_pos_left = REF_UI_MODE_OFFSET_3_LEFT;
            box_pos_right = REF_UI_MODE_OFFSET_3_RIGHT;
            break;
          case Component::CMD::Rotor:
            box_pos_left = REF_UI_MODE_OFFSET_4_LEFT;
            box_pos_right = REF_UI_MODE_OFFSET_4_RIGHT;
            break;
          default:
            box_pos_left = 0.0f;
            box_pos_right = 0.0f;
            break;
        }
        if (box_pos_left != 0.0f && box_pos_right != 0.0f) {
          ui_draw_rectangle(
              &ele, "2", graphic_op, UI_GRAPHIC_LAYER_CHASSIS, UI_GREEN,
              UI_DEFAULT_WIDTH,
              (uint16_t)(kW * REF_UI_RIGHT_START_W + box_pos_left),
              (uint16_t)(kH * REF_UI_MODE_LINE1_H + REF_UI_BOX_UP_OFFSET),
              (uint16_t)(kW * REF_UI_RIGHT_START_W + box_pos_right),
              (uint16_t)(kH * REF_UI_MODE_LINE1_H + REF_UI_BOX_BOT_OFFSET));

          ui_stash_graphic(&(this->ui_.ui), &ele);
        }
        break;
      }

      default:
        this->ui_.ui.refresh_fsm = 0;
        if (graphic_op == UI_GRAPHIC_OP_ADD) graphic_op = UI_GRAPHIC_OP_REWRITE;
    }
  }

#elif UI_MODE_REMOTE
  if (this->ui_slow_refresh_.Take(0)) {
    // TODO:
  }

  if (this->ui_fast_refresh_.Take(0)) {
    // TODO:
  }

#endif
  return true;
}

bool Referee::StartTrans() {
  if (this->packet.data_ != NULL && this->packet.size_ > 0) {
    memcpy(txbuf, this->packet.data_, this->packet.size_);
    vPortFree(this->packet.data_);
    this->packet.data_ = NULL;
  } else {
    this->packet_sent_.Give();
    return false;
  }

  if (bsp_uart_transmit(BSP_UART_REF, txbuf,
                        static_cast<uint16_t>(this->packet.size_),
                        false) == BSP_OK) {
    return true;
  } else {
    this->packet_sent_.Give();
    return false;
  }
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
