/*
  裁判系统抽象。
*/

/* Includes ----------------------------------------------------------------- */
#include "referee.h"

#include <string.h>

#include "bsp/delay.h"
#include "bsp/mm.h"
#include "bsp/uart.h"
#include "component/crc16.h"
#include "component/crc8.h"
#include "component/user_math.h"

/* Private define ----------------------------------------------------------- */
#define REF_HEADER_SOF (0xA5)
#define REF_LEN_RX_BUFF (0xFF)

#define REF_UI_FAST_REFRESH_FREQ (50)   /* 静态元素刷新频率 */
#define REF_UI_SLOW_REFRESH_FREQ (0.2f) /* 动态元素刷新频率 */

#define REF_UI_BOX_UP_OFFSET (4)
#define REF_UI_BOX_BOT_OFFSET (-14)

#define REF_UI_RIGHT_START_W (0.85f)

#define REF_UI_MODE_LINE1_H (0.7f)
#define REF_UI_MODE_LINE2_H (0.68f)
#define REF_UI_MODE_LINE3_H (0.66f)
#define REF_UI_MODE_LINE4_H (0.64f)

/* Private macro ------------------------------------------------------------ */

#define REF_SET_STATUS(ref, stat) ((ref).status = (stat))

/* Private typedef ---------------------------------------------------------- */

typedef struct __packed {
  Referee_Header_t header;
  uint16_t cmd_id;
  Referee_InterStudentHeader_t student_header;
} Referee_UiPacketHead_t;

/* Private variables -------------------------------------------------------- */

static volatile uint32_t drop_message = 0;

static uint8_t rxbuf[REF_LEN_RX_BUFF];

static Referee_t *gref;

static bool inited = false;

/* Private function  -------------------------------------------------------- */

static void Referee_RxCpltCallback(void) {
  osThreadFlagsSet(gref->thread_alert, SIGNAL_REFEREE_RAW_REDY);
}

static void Referee_IdleLineCallback(void) {
  HAL_UART_AbortReceive_IT(BSP_UART_GetHandle(BSP_UART_REF));
}

static void Referee_AbortRxCpltCallback(void) {
  osThreadFlagsSet(gref->thread_alert, SIGNAL_REFEREE_RAW_REDY);
}

static void Referee_TxCpltCallback(void) {
  BSP_Free(gref->packet.data);
  osThreadFlagsSet(gref->thread_alert, SIGNAL_REFEREE_PACKET_SENT);
}

static void RefereeFastRefreshTimerCallback(void *arg) {
  UNUSED(arg);
  osThreadFlagsSet(gref->thread_alert, SIGNAL_REFEREE_FAST_REFRESH_UI);
}

static void RefereeSlowRefreshTimerCallback(void *arg) {
  UNUSED(arg);
  osThreadFlagsSet(gref->thread_alert, SIGNAL_REFEREE_SLOW_REFRESH_UI);
}

static int8_t Referee_MoveData(void *data, void *tmp, uint32_t len) {
  if (len <= 0 || data == NULL || tmp == NULL) return DEVICE_ERR;
  memcpy(tmp, data, len);
  memset(data, 0, len);
  return DEVICE_OK;
}

static int8_t Referee_SetPacketHeader(Referee_Header_t *header,
                                      uint16_t data_length) {
  header->sof = REF_HEADER_SOF;
  header->data_length = data_length;
  header->crc8 =
      CRC8_Calc((const uint8_t *)&header,
                sizeof(Referee_Header_t) - sizeof(uint8_t), CRC8_INIT);
  return DEVICE_OK;
}

static int8_t Referee_SetUiHeader(Referee_InterStudentHeader_t *header,
                                  const Referee_StudentCMDID_t cmd_id,
                                  Referee_RobotID_t robot_id) {
  header->cmd_id = cmd_id;
  header->id_sender = robot_id;
  if (robot_id > 100) {
    header->id_receiver = robot_id - 100 + 0x0160;
  } else {
    header->id_receiver = robot_id + 0x0100;
  }
  return 0;
}

/* Exported functions ------------------------------------------------------- */

int8_t Referee_Init(Referee_t *ref, const UI_Screen_t *screen) {
  if (ref == NULL) return DEVICE_ERR_NULL;
  if (inited) return DEVICE_ERR_INITED;

  if ((gref->thread_alert = osThreadGetId()) == NULL) return DEVICE_ERR_NULL;
  gref = ref;

  ref->ui.screen = screen;

  BSP_UART_RegisterCallback(BSP_UART_REF, BSP_UART_RX_CPLT_CB,
                            Referee_RxCpltCallback);
  BSP_UART_RegisterCallback(BSP_UART_REF, BSP_UART_ABORT_RX_CPLT_CB,
                            Referee_AbortRxCpltCallback);
  BSP_UART_RegisterCallback(BSP_UART_REF, BSP_UART_IDLE_LINE_CB,
                            Referee_IdleLineCallback);
  BSP_UART_RegisterCallback(BSP_UART_REF, BSP_UART_TX_CPLT_CB,
                            Referee_TxCpltCallback);

  uint32_t fast_period_ms = (uint32_t)(1000.0f / REF_UI_FAST_REFRESH_FREQ);
  uint32_t slow_period_ms = (uint32_t)(1000.0f / REF_UI_SLOW_REFRESH_FREQ);

  ref->ui_fast_timer_id =
      osTimerNew(RefereeFastRefreshTimerCallback, osTimerPeriodic, NULL, NULL);

  ref->ui_slow_timer_id =
      osTimerNew(RefereeSlowRefreshTimerCallback, osTimerPeriodic, NULL, NULL);

  osTimerStart(ref->ui_fast_timer_id, fast_period_ms);
  osTimerStart(ref->ui_slow_timer_id, slow_period_ms);

  __HAL_UART_ENABLE_IT(BSP_UART_GetHandle(BSP_UART_REF), UART_IT_IDLE);

  inited = true;
  return 0;
}

int8_t Referee_Restart(void) {
  __HAL_UART_DISABLE(BSP_UART_GetHandle(BSP_UART_REF));
  __HAL_UART_ENABLE(BSP_UART_GetHandle(BSP_UART_REF));
  return 0;
}

void Referee_HandleOffline(Referee_t *referee) {
  referee->status = REF_STATUS_OFFLINE;
}

int8_t Referee_StartReceiving(Referee_t *ref) {
  UNUSED(ref);
  if (HAL_UART_Receive_DMA(BSP_UART_GetHandle(BSP_UART_REF), rxbuf,
                           REF_LEN_RX_BUFF) == HAL_OK)
    return DEVICE_OK;
  return DEVICE_ERR;
}

bool Referee_WaitRecvCplt(uint32_t timeout) {
  return (osThreadFlagsWait(SIGNAL_REFEREE_RAW_REDY, osFlagsWaitAll, timeout) ==
          SIGNAL_REFEREE_RAW_REDY);
}

int8_t Referee_Parse(Referee_t *ref) {
  REF_SET_STATUS(*ref, REF_STATUS_RUNNING);
  uint32_t data_length =
      REF_LEN_RX_BUFF -
      __HAL_DMA_GET_COUNTER(BSP_UART_GetHandle(BSP_UART_REF)->hdmarx);

  uint8_t index = 0;
  uint8_t packet_shift;
  uint8_t packet_length;

  while (index < data_length && rxbuf[index] == REF_HEADER_SOF) {
    packet_shift = index;
    Referee_Header_t *header = (Referee_Header_t *)(rxbuf + index);
    index += sizeof(*header);
    if (index - packet_shift >= data_length) goto error;

    if (!CRC8_Verify((uint8_t *)header, sizeof(*header))) goto error;

    if (header->sof != REF_HEADER_SOF) goto error;

    Referee_CMDID_t *cmd_id = (Referee_CMDID_t *)(rxbuf + index);
    index += sizeof(*cmd_id);
    if (index - packet_shift >= data_length) goto error;

    void *target = (rxbuf + index);
    void *origin;
    size_t size;

    switch (*cmd_id) {
      case REF_CMD_ID_GAME_STATUS:
        origin = &(ref->game_status);
        size = sizeof(ref->game_status);
        break;
      case REF_CMD_ID_GAME_RESULT:
        origin = &(ref->game_result);
        size = sizeof(ref->game_result);
        break;
      case REF_CMD_ID_GAME_ROBOT_HP:
        origin = &(ref->game_robot_hp);
        size = sizeof(ref->game_robot_hp);
        break;
      case REF_CMD_ID_DART_STATUS:
        origin = &(ref->dart_status);
        size = sizeof(ref->dart_status);
        break;
      case REF_CMD_ID_ICRA_ZONE_STATUS:
        origin = &(ref->icra_zone);
        size = sizeof(ref->icra_zone);
        break;
      case REF_CMD_ID_FIELD_EVENTS:
        origin = &(ref->field_event);
        size = sizeof(ref->field_event);
        break;
      case REF_CMD_ID_SUPPLY_ACTION:
        origin = &(ref->supply_action);
        size = sizeof(ref->supply_action);
        break;
      case REF_CMD_ID_WARNING:
        origin = &(ref->warning);
        size = sizeof(ref->warning);
        break;
      case REF_CMD_ID_DART_COUNTDOWN:
        origin = &(ref->dart_countdown);
        size = sizeof(ref->dart_countdown);
        break;
      case REF_CMD_ID_ROBOT_STATUS:
        origin = &(ref->robot_status);
        size = sizeof(ref->robot_status);
        break;
      case REF_CMD_ID_POWER_HEAT_DATA:
        origin = &(ref->power_heat);
        size = sizeof(ref->power_heat);
        break;
      case REF_CMD_ID_ROBOT_POS:
        origin = &(ref->robot_pos);
        size = sizeof(ref->robot_pos);
        break;
      case REF_CMD_ID_ROBOT_BUFF:
        origin = &(ref->robot_buff);
        size = sizeof(ref->robot_buff);
        break;
      case REF_CMD_ID_DRONE_ENERGY:
        origin = &(ref->drone_energy);
        size = sizeof(ref->drone_energy);
        break;
      case REF_CMD_ID_ROBOT_DMG:
        origin = &(ref->robot_danage);
        size = sizeof(ref->robot_danage);
        break;
      case REF_CMD_ID_LAUNCHER_DATA:
        origin = &(ref->launcher_data);
        size = sizeof(ref->launcher_data);
        break;
      case REF_CMD_ID_BULLET_REMAINING:
        origin = &(ref->bullet_remain);
        size = sizeof(ref->bullet_remain);
        break;
      case REF_CMD_ID_RFID:
        origin = &(ref->rfid);
        size = sizeof(ref->rfid);
        break;
      case REF_CMD_ID_DART_CLIENT:
        origin = &(ref->dart_client);
        size = sizeof(ref->dart_client);
        break;
      case REF_CMD_ID_CLIENT_MAP:
        origin = &(ref->client_map);
        size = sizeof(ref->client_map);
      case REF_CMD_ID_KEYBOARD_MOUSE:
        origin = &(ref->keyboard_mouse);
        size = sizeof(ref->keyboard_mouse);
      default:
        return DEVICE_ERR;
    }
    packet_length = sizeof(Referee_Header_t) + sizeof(Referee_CMDID_t) + size +
                    sizeof(Referee_Tail_t);
    index += size;
    if (index - packet_shift >= data_length) goto error;

    index += sizeof(Referee_Tail_t);
    if (index - packet_shift != packet_length) goto error;

    if (CRC16_Verify((uint8_t *)(rxbuf + packet_shift), packet_length))
      memcpy(origin, target, size);
    else
      goto error;
  }
  return DEVICE_OK;

error:
  drop_message++;
  return DEVICE_ERR;
}

uint8_t Referee_RefreshUI(Referee_t *ref) {
  UI_Ele_t ele;
  UI_String_t string;

  const float kW = ref->ui.screen->width;
  const float kH = ref->ui.screen->height;

  float box_pos_h = 0.0f;

  /* UI动态元素刷新 */
  if (osThreadFlagsGet() & SIGNAL_REFEREE_FAST_REFRESH_UI) {
    osThreadFlagsClear(SIGNAL_REFEREE_FAST_REFRESH_UI);
    /* 使用状态机算法，每次更新一个图层 */
    switch (ref->ui.refresh_fsm) {
      case 0: {
        ref->ui.refresh_fsm++;

        /* 更新云台底盘相对方位 */
        const float kLEN = 46;
        UI_DrawLine(&ele, "6", UI_GRAPHIC_OP_REWRITE, UI_GRAPHIC_LAYER_CHASSIS,
                    UI_GREEN, UI_DEFAULT_WIDTH * 12, kW * 0.4, kH * 0.2,
                    kW * 0.4 + sin(ref->chassis_ui.angle) * kLEN,
                    kH * 0.2 + cos(ref->chassis_ui.angle) * kLEN);

        UI_StashGraphic(&(ref->ui), &ele);

        /* 更新底盘模式选择框 */
        switch (ref->chassis_ui.mode) {
          case CHASSIS_MODE_FOLLOW_GIMBAL:
            box_pos_h = REF_UI_MODE_LINE2_H;
            break;
          case CHASSIS_MODE_FOLLOW_GIMBAL_35:
            box_pos_h = REF_UI_MODE_LINE3_H;
            break;
          case CHASSIS_MODE_ROTOR:
            box_pos_h = REF_UI_MODE_LINE4_H;
            break;
          default:
            box_pos_h = 0.0f;
            break;
        }
        if (box_pos_h != 0.0f) {
          UI_DrawRectangle(&ele, "8", UI_GRAPHIC_OP_REWRITE,
                           UI_GRAPHIC_LAYER_CHASSIS, UI_GREEN, UI_DEFAULT_WIDTH,
                           kW * REF_UI_RIGHT_START_W - 6,
                           kH * box_pos_h + REF_UI_BOX_UP_OFFSET,
                           kW * REF_UI_RIGHT_START_W + 44,
                           kH * box_pos_h + REF_UI_BOX_BOT_OFFSET);

          UI_StashGraphic(&(ref->ui), &ele);
        }
        break;
      }
      case 1:
        ref->ui.refresh_fsm++;
        /* 更新电容状态 */
        switch (ref->cap_ui.status) {
          case CAN_CAP_STATUS_OFFLINE:
            UI_DrawArc(&ele, "9", UI_GRAPHIC_OP_REWRITE, UI_GRAPHIC_LAYER_CAP,
                       UI_YELLOW, 0, 360, UI_DEFAULT_WIDTH * 5, kW * 0.6,
                       kH * 0.2, 50, 50);
            break;
          default:
            UI_DrawArc(&ele, "9", UI_GRAPHIC_OP_REWRITE, UI_GRAPHIC_LAYER_CAP,
                       UI_GREEN, 0, ref->cap_ui.percentage * 360,
                       UI_DEFAULT_WIDTH * 5, kW * 0.6, kH * 0.2, 50, 50);
            break;
        }
        UI_StashGraphic(&(ref->ui), &ele);
        break;
      case 2: {
        ref->ui.refresh_fsm++;

        /* 更新云台模式选择框 */
        switch (ref->gimbal_ui.mode) {
          case GIMBAL_MODE_RELAX:
            box_pos_h = REF_UI_MODE_LINE2_H;
            break;
          case GIMBAL_MODE_ABSOLUTE:
            box_pos_h = REF_UI_MODE_LINE3_H;
            break;
          case GIMBAL_MODE_RELATIVE:
            box_pos_h = REF_UI_MODE_LINE4_H;
            break;
          default:
            box_pos_h = 0.0f;
            break;
        }
        UI_DrawRectangle(&ele, "a", UI_GRAPHIC_OP_REWRITE,
                         UI_GRAPHIC_LAYER_GIMBAL, UI_GREEN, UI_DEFAULT_WIDTH,
                         kW * REF_UI_RIGHT_START_W + 54,
                         kH * box_pos_h + REF_UI_BOX_UP_OFFSET,
                         kW * REF_UI_RIGHT_START_W + 102,
                         kH * box_pos_h + REF_UI_BOX_BOT_OFFSET);
        UI_StashGraphic(&(ref->ui), &ele);
        break;
      }
      case 3: {
        ref->ui.refresh_fsm++;

        /* 更新发谁器模式选择框 */
        switch (ref->launcher_ui.mode) {
          case LAUNCHER_MODE_RELAX:
            box_pos_h = REF_UI_MODE_LINE2_H;
            break;
          case LAUNCHER_MODE_SAFE:
            box_pos_h = REF_UI_MODE_LINE3_H;
            break;
          case LAUNCHER_MODE_LOADED:
            box_pos_h = REF_UI_MODE_LINE4_H;
            break;
          default:
            box_pos_h = 0.0f;
            break;
        }
        UI_DrawRectangle(&ele, "b", UI_GRAPHIC_OP_REWRITE,
                         UI_GRAPHIC_LAYER_LAUNCHER, UI_GREEN, UI_DEFAULT_WIDTH,
                         kW * REF_UI_RIGHT_START_W + 114,
                         kH * box_pos_h + REF_UI_BOX_UP_OFFSET,
                         kW * REF_UI_RIGHT_START_W + 162,
                         kH * box_pos_h + REF_UI_BOX_BOT_OFFSET);
        UI_StashGraphic(&(ref->ui), &ele);

        /* 更新开火模式选择框 */
        switch (ref->launcher_ui.fire) {
          case FIRE_MODE_SINGLE:
            box_pos_h = REF_UI_MODE_LINE2_H;
            break;
          case FIRE_MODE_BURST:
            box_pos_h = REF_UI_MODE_LINE3_H;
            break;
          case FIRE_MODE_CONT:
            box_pos_h = REF_UI_MODE_LINE4_H;
          default:
            break;
        }
        UI_DrawRectangle(&ele, "f", UI_GRAPHIC_OP_REWRITE,
                         UI_GRAPHIC_LAYER_LAUNCHER, UI_GREEN, UI_DEFAULT_WIDTH,
                         kW * REF_UI_RIGHT_START_W + 174,
                         kH * box_pos_h + REF_UI_BOX_UP_OFFSET,
                         kW * REF_UI_RIGHT_START_W + 222,
                         kH * box_pos_h + REF_UI_BOX_BOT_OFFSET);
        UI_StashGraphic(&(ref->ui), &ele);
        break;
      }
      case 4:
        ref->ui.refresh_fsm++;

        switch (ref->cmd_ui.ctrl_method) {
          case CMD_METHOD_MOUSE_KEYBOARD:
            UI_DrawRectangle(&ele, "c", UI_GRAPHIC_OP_REWRITE,
                             UI_GRAPHIC_LAYER_CMD, UI_GREEN, UI_DEFAULT_WIDTH,
                             kW * REF_UI_RIGHT_START_W + 96,
                             kH * 0.4 + REF_UI_BOX_UP_OFFSET,
                             kW * REF_UI_RIGHT_START_W + 120,
                             kH * 0.4 + REF_UI_BOX_BOT_OFFSET);
            break;
          case CMD_METHOD_JOYSTICK_SWITCH:
            UI_DrawRectangle(
                &ele, "c", UI_GRAPHIC_OP_REWRITE, UI_GRAPHIC_LAYER_CMD,
                UI_GREEN, UI_DEFAULT_WIDTH, kW * REF_UI_RIGHT_START_W + 56,
                kH * 0.4 + REF_UI_BOX_UP_OFFSET, kW * REF_UI_RIGHT_START_W + 80,
                kH * 0.4 + REF_UI_BOX_BOT_OFFSET);
            break;
        }
        UI_StashGraphic(&(ref->ui), &ele);
        break;

      default:
        ref->ui.refresh_fsm = 0;
    }
  }

  /* UI静态元素刷新 */
  if (osThreadFlagsGet() & SIGNAL_REFEREE_SLOW_REFRESH_UI) {
    osThreadFlagsClear(SIGNAL_REFEREE_SLOW_REFRESH_UI);
    UI_DrawString(&string, "1", UI_GRAPHIC_OP_REWRITE, UI_GRAPHIC_LAYER_CONST,
                  UI_GREEN, UI_DEFAULT_WIDTH * 10, 80, UI_CHAR_DEFAULT_WIDTH,
                  kW * REF_UI_RIGHT_START_W, kH * REF_UI_MODE_LINE1_H,
                  "CHAS  FLLW  FL35  ROTR");
    UI_StashString(&(ref->ui), &string);

    UI_DrawString(&string, "2", UI_GRAPHIC_OP_REWRITE, UI_GRAPHIC_LAYER_CONST,
                  UI_GREEN, UI_DEFAULT_WIDTH * 10, 80, UI_CHAR_DEFAULT_WIDTH,
                  kW * REF_UI_RIGHT_START_W, kH * REF_UI_MODE_LINE2_H,
                  "GMBL  RELX  ABSL  RLTV");
    UI_StashString(&(ref->ui), &string);

    UI_DrawString(&string, "3", UI_GRAPHIC_OP_REWRITE, UI_GRAPHIC_LAYER_CONST,
                  UI_GREEN, UI_DEFAULT_WIDTH * 10, 80, UI_CHAR_DEFAULT_WIDTH,
                  kW * REF_UI_RIGHT_START_W, kH * REF_UI_MODE_LINE3_H,
                  "SHOT  RELX  SAFE  LOAD");
    UI_StashString(&(ref->ui), &string);

    UI_DrawString(&string, "4", UI_GRAPHIC_OP_REWRITE, UI_GRAPHIC_LAYER_CONST,
                  UI_GREEN, UI_DEFAULT_WIDTH * 10, 80, UI_CHAR_DEFAULT_WIDTH,
                  kW * REF_UI_RIGHT_START_W, kH * REF_UI_MODE_LINE4_H,
                  "FIRE  SNGL  BRST  CONT");
    UI_StashString(&(ref->ui), &string);

    UI_DrawLine(&ele, "5", UI_GRAPHIC_OP_REWRITE, UI_GRAPHIC_LAYER_CONST,
                UI_GREEN, UI_DEFAULT_WIDTH * 3, kW * 0.4, kH * 0.2, kW * 0.4,
                kH * 0.2 + 50);
    UI_StashGraphic(&(ref->ui), &ele);

    UI_DrawString(&string, "d", UI_GRAPHIC_OP_REWRITE, UI_GRAPHIC_LAYER_CONST,
                  UI_GREEN, UI_DEFAULT_WIDTH * 10, 80, UI_CHAR_DEFAULT_WIDTH,
                  kW * REF_UI_RIGHT_START_W, kH * 0.4, "CTRL  JS  KM");
    UI_StashString(&(ref->ui), &string);

    UI_DrawString(&string, "e", UI_GRAPHIC_OP_REWRITE, UI_GRAPHIC_LAYER_CONST,
                  UI_GREEN, UI_DEFAULT_WIDTH * 20, 80,
                  UI_CHAR_DEFAULT_WIDTH * 2, kW * 0.6 - 26, kH * 0.2 + 10,
                  "CAP");
    UI_StashString(&(ref->ui), &string);
  }

  return 0;
}

/**
 * @brief 组装UI包
 *
 * @param ui UI数据
 * @param ref 裁判系统数据
 * @return int8_t 0代表成功
 */
int8_t Referee_PackUiPacket(Referee_t *ref) {
  UI_Ele_t *ele = NULL;
  UI_String_t string;
  UI_Del_t del;

  Referee_StudentCMDID_t ui_cmd_id;
  static const size_t kSIZE_DATA_HEADER = sizeof(Referee_InterStudentHeader_t);
  size_t size_data_content;
  static const size_t kSIZE_PACKET_CRC = sizeof(uint16_t);
  void *origin = NULL;

  if (!UI_PopDel(&del, &(ref->ui))) {
    origin = &del;
    size_data_content = sizeof(UI_Del_t);
    ui_cmd_id = REF_STDNT_CMD_ID_UI_DEL;
  } else if (ref->ui.stack.size.graphic) { /* 绘制图形 */
    if (ref->ui.stack.size.graphic <= 1) {
      size_data_content = sizeof(UI_Ele_t) * 1;
      ui_cmd_id = REF_STDNT_CMD_ID_UI_DRAW1;

    } else if (ref->ui.stack.size.graphic <= 2) {
      size_data_content = sizeof(UI_Ele_t) * 2;
      ui_cmd_id = REF_STDNT_CMD_ID_UI_DRAW2;

    } else if (ref->ui.stack.size.graphic <= 5) {
      size_data_content = sizeof(UI_Ele_t) * 5;
      ui_cmd_id = REF_STDNT_CMD_ID_UI_DRAW5;

    } else if (ref->ui.stack.size.graphic <= 7) {
      size_data_content = sizeof(UI_Ele_t) * 7;
      ui_cmd_id = REF_STDNT_CMD_ID_UI_DRAW7;

    } else {
      return DEVICE_ERR;
    }
    ele = BSP_Malloc(size_data_content);
    UI_Ele_t *cursor = ele;
    while (!UI_PopGraphic(cursor, &(ref->ui))) {
      cursor++;
    }
    origin = ele;
  } else if (!UI_PopString(&string, &(ref->ui))) { /* 绘制字符 */
    origin = &string;
    size_data_content = sizeof(UI_String_t);
    ui_cmd_id = REF_STDNT_CMD_ID_UI_STR;
  } else {
    return DEVICE_ERR;
  }

  ref->packet.size =
      sizeof(Referee_UiPacketHead_t) + size_data_content + kSIZE_PACKET_CRC;

  ref->packet.data = BSP_Malloc(ref->packet.size);

  Referee_UiPacketHead_t *packet_head =
      (Referee_UiPacketHead_t *)(ref->packet.data);

  Referee_SetPacketHeader(&(packet_head->header),
                          kSIZE_DATA_HEADER + size_data_content);
  packet_head->cmd_id = REF_CMD_ID_INTER_STUDENT;
  Referee_SetUiHeader(&(packet_head->student_header), ui_cmd_id,
                      ref->robot_status.robot_id);
  Referee_MoveData(origin, ref->packet.data + sizeof(Referee_UiPacketHead_t),
                   size_data_content);
  BSP_Free(ele);
  uint16_t *crc = (uint16_t *)ref->packet.data +
                  sizeof(Referee_UiPacketHead_t) + size_data_content;
  *crc = CRC16_Calc((const uint8_t *)ref->packet.data,
                    ref->packet.size - kSIZE_PACKET_CRC, CRC16_INIT);

  return DEVICE_OK;
}

int8_t Referee_StartTransmit(const Referee_t *ref) {
  if (HAL_UART_Transmit_DMA(BSP_UART_GetHandle(BSP_UART_REF), ref->packet.data,
                            ref->packet.size) == HAL_OK) {
    return DEVICE_OK;
  }
  return DEVICE_ERR;
}

bool Referee_WaitTransCplt(uint32_t timeout) {
  return (osThreadFlagsWait(SIGNAL_REFEREE_PACKET_SENT, osFlagsWaitAll,
                            timeout) == SIGNAL_REFEREE_PACKET_SENT);
}

uint8_t Referee_PackForChassis(Referee_ForChassis_t *c_ref,
                               const Referee_t *ref) {
  c_ref->chassis_power_limit = ref->robot_status.chassis_power_limit;
  c_ref->chassis_pwr_buff = ref->power_heat.chassis_pwr_buff;
  c_ref->status = ref->status;
  return 0;
}

uint8_t Referee_PackForLauncher(Referee_ForLauncher_t *l_ref, Referee_t *ref) {
  memcpy(&(l_ref->power_heat), &(ref->power_heat), sizeof(l_ref->power_heat));
  memcpy(&(l_ref->robot_status), &(ref->robot_status),
         sizeof(l_ref->robot_status));
  memcpy(&(l_ref->launcher_data), &(ref->launcher_data),
         sizeof(l_ref->launcher_data));
  l_ref->status = ref->status;
  return 0;
}

uint8_t Referee_PackForCap(Referee_ForCap_t *cap_ref, const Referee_t *ref) {
  cap_ref->chassis_power_limit = ref->robot_status.chassis_power_limit;
  cap_ref->chassis_pwr_buff = ref->power_heat.chassis_pwr_buff;
  cap_ref->chassis_watt = ref->power_heat.chassis_watt;
  cap_ref->status = ref->status;
  return 0;
}

uint8_t Referee_PackForAI(Referee_ForAI_t *ai_ref, const Referee_t *ref) {
  ai_ref->status = ref->status;
  return 0;
}