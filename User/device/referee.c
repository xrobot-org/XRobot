/*
  裁判系统抽象。
*/

/* Includes ----------------------------------------------------------------- */
#include "referee.h"

#include <string.h>

#include "bsp\delay.h"
#include "bsp\uart.h"
#include "component\crc16.h"
#include "component\crc8.h"
#include "component\user_math.h"

/* Private define ----------------------------------------------------------- */
#define REF_HEADER_SOF (0xA5)
#define REF_LEN_RX_BUFF (0xFF)

#define REF_UI_MAX_GRAPIC_NUM (7)
#define REF_UI_MAX_STRING_NUM (3)
#define REF_UI_MAX_DEL_NUM (3)

/* Private macro ------------------------------------------------------------ */
/* Private typedef ---------------------------------------------------------- */
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

/* Exported functions ------------------------------------------------------- */
int8_t Referee_Init(Referee_t *ref, osThreadId_t thread_alert) {
  if (ref == NULL) return DEVICE_ERR_NULL;
  if (inited) return DEVICE_ERR_INITED;

  ref->thread_alert = thread_alert;

  BSP_UART_RegisterCallback(BSP_UART_REF, BSP_UART_RX_CPLT_CB,
                            Referee_RxCpltCallback);
  BSP_UART_RegisterCallback(BSP_UART_REF, BSP_UART_ABORT_RX_CPLT_CB,
                            Referee_AbortRxCpltCallback);
  BSP_UART_RegisterCallback(BSP_UART_REF, BSP_UART_IDLE_LINE_CB,
                            Referee_IdleLineCallback);

  __HAL_UART_ENABLE_IT(BSP_UART_GetHandle(BSP_UART_REF), UART_IT_IDLE);

  gref = ref;
  inited = true;
  return 0;
}

int8_t Referee_Restart(void) {
  __HAL_UART_DISABLE(BSP_UART_GetHandle(BSP_UART_DR16));
  __HAL_UART_ENABLE(BSP_UART_GetHandle(BSP_UART_DR16));
  return 0;
}

int8_t Referee_StartReceiving(Referee_t *ref) {
  (void)ref;
  if (HAL_UART_Receive_DMA(BSP_UART_GetHandle(BSP_UART_REF), rxbuf,
                           REF_LEN_RX_BUFF) == HAL_OK)
    return DEVICE_OK;
  return DEVICE_ERR;
}

void Referee_HandleOffline(Referee_t *referee) {
  referee->ref_status = REF_STATUS_OFFLINE;
}

int8_t Referee_Parse(Referee_t *ref) {
  REF_SWITCH_STATUS(*ref, REF_STATUS_RUNNING);
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
      case REF_CMD_ID_REQUEST_SUPPLY:
        origin = &(ref->request_supply);
        size = sizeof(ref->request_supply);
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
      case REF_CMD_ID_SHOOT_DATA:
        origin = &(ref->shoot_data);
        size = sizeof(ref->shoot_data);
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
      case REF_CMD_ID_INTER_STUDENT_CUSTOM:
        origin = &(ref->custom);
        size = sizeof(ref->custom);
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

int8_t Referee_StartSend(uint8_t *data, uint32_t len) {
  if (HAL_UART_Transmit_DMA(BSP_UART_GetHandle(BSP_UART_REF), data,
                            (size_t)len) == HAL_OK) {
    return DEVICE_OK;
  } else
    return DEVICE_ERR;
}

int8_t Referee_MoveData(void *data, void *tmp, uint32_t len) {
  if (len <= 0 || data == NULL || tmp == NULL) return DEVICE_ERR;
  memcpy(tmp, (const void *)data, (size_t)len);
  memset(data, 0, (size_t)len);
  return DEVICE_OK;
}

int8_t Referee_SetHeader(Referee_Interactive_Header_t *header,
                         Referee_StudentCMDID_t cmd_id, uint8_t sender_id) {
  header->data_cmd_id = cmd_id;
  if (sender_id <= REF_BOT_RED_RADER) switch (sender_id) {
      case REF_BOT_RED_HERO:
        header->sender_ID = REF_BOT_RED_HERO;
        header->receiver_ID = REF_CL_RED_HERO;
        break;
      case REF_BOT_RED_ENGINEER:
        header->sender_ID = REF_BOT_RED_ENGINEER;
        header->receiver_ID = REF_CL_RED_ENGINEER;
        break;
      case REF_BOT_RED_INFANTRY_1:
        header->sender_ID = REF_BOT_RED_INFANTRY_1;
        header->receiver_ID = REF_CL_RED_INFANTRY_1;
        break;
      case REF_BOT_RED_INFANTRY_2:
        header->sender_ID = REF_BOT_RED_INFANTRY_2;
        header->receiver_ID = REF_CL_RED_INFANTRY_2;
        break;
      case REF_BOT_RED_INFANTRY_3:
        header->sender_ID = REF_BOT_RED_INFANTRY_3;
        header->receiver_ID = REF_CL_RED_INFANTRY_3;
        break;
      case REF_BOT_RED_DRONE:
        header->sender_ID = REF_BOT_RED_DRONE;
        header->receiver_ID = REF_CL_RED_DRONE;
        break;
      case REF_BOT_RED_SENTRY:
        header->sender_ID = REF_BOT_RED_SENTRY;
        break;
      case REF_BOT_RED_RADER:
        header->sender_ID = REF_BOT_RED_RADER;
        break;
      default:
        return -1;
    }
  else
    switch (sender_id) {
      case REF_BOT_BLU_HERO:
        header->sender_ID = REF_BOT_BLU_HERO;
        header->receiver_ID = REF_CL_BLU_HERO;
        break;
      case REF_BOT_BLU_ENGINEER:
        header->sender_ID = REF_BOT_BLU_ENGINEER;
        header->receiver_ID = REF_CL_BLU_ENGINEER;
        break;
      case REF_BOT_BLU_INFANTRY_1:
        header->sender_ID = REF_BOT_BLU_INFANTRY_1;
        header->receiver_ID = REF_CL_BLU_INFANTRY_1;
        break;
      case REF_BOT_BLU_INFANTRY_2:
        header->sender_ID = REF_BOT_BLU_INFANTRY_2;
        header->receiver_ID = REF_CL_BLU_INFANTRY_2;
        break;
      case REF_BOT_BLU_INFANTRY_3:
        header->sender_ID = REF_BOT_BLU_INFANTRY_3;
        header->receiver_ID = REF_CL_BLU_INFANTRY_3;
        break;
      case REF_BOT_BLU_DRONE:
        header->sender_ID = REF_BOT_BLU_DRONE;
        header->receiver_ID = REF_CL_BLU_DRONE;
        break;
      case REF_BOT_BLU_SENTRY:
        header->sender_ID = REF_BOT_BLU_SENTRY;
        break;
      case REF_BOT_BLU_RADER:
        header->sender_ID = REF_BOT_BLU_RADER;
        break;
      default:
        return -1;
    }
  return 0;
}

int8_t Referee_PackUI(Referee_UI_t *ui, Referee_t *ref) {
  static uint8_t send_data[sizeof(Referee_UI_Drawgrapic_7_t)] = {0};
  uint16_t size;
  if (ui->grapic_counter != 0) {
    switch (ui->grapic_counter) {
      case 1:
        size = sizeof(Referee_UI_Drawgrapic_1_t);
        Referee_UI_Drawgrapic_1_t *address_1 =
            (Referee_UI_Drawgrapic_1_t *)send_data;
        address_1->header.sof = REF_HEADER_SOF;
        address_1->header.data_length =
            sizeof(UI_Drawgrapic_1_t) + sizeof(Referee_Interactive_Header_t);
        address_1->header.crc8 =
            CRC8_Calc((const uint8_t *)&(address_1->header),
                      sizeof(Referee_Header_t) - sizeof(uint8_t), CRC8_INIT);
        address_1->cmd_id = REF_CMD_ID_INTER_STUDENT;
        Referee_SetHeader(&(address_1->IA_header), REF_STDNT_CMD_ID_UI_DRAW1,
                          ref->robot_status.robot_id);
        Referee_MoveData(&(ui->grapic), &(address_1->data.grapic),
                         sizeof(UI_Drawgrapic_1_t));
        address_1->crc16 = CRC16_Calc(
            (const uint8_t *)address_1,
            sizeof(Referee_UI_Drawgrapic_1_t) - sizeof(uint16_t), CRC16_INIT);
        break;
      case 2:
        size = sizeof(Referee_UI_Drawgrapic_2_t);
        Referee_UI_Drawgrapic_2_t *address_2 =
            (Referee_UI_Drawgrapic_2_t *)send_data;
        address_2->header.sof = REF_HEADER_SOF;
        address_2->header.data_length =
            sizeof(UI_Drawgrapic_2_t) + sizeof(Referee_Interactive_Header_t);
        address_2->header.crc8 =
            CRC8_Calc((const uint8_t *)&(address_2->header),
                      sizeof(Referee_Header_t) - sizeof(uint8_t), CRC8_INIT);
        address_2->cmd_id = REF_CMD_ID_INTER_STUDENT;
        Referee_SetHeader(&(address_2->IA_header), REF_STDNT_CMD_ID_UI_DRAW2,
                          ref->robot_status.robot_id);
        Referee_MoveData(&(ui->grapic), &(address_2->data.grapic),
                         sizeof(UI_Drawgrapic_2_t));
        address_2->crc16 = CRC16_Calc(
            (const uint8_t *)address_2,
            sizeof(Referee_UI_Drawgrapic_2_t) - sizeof(uint16_t), CRC16_INIT);
        break;
      case 3:
      case 4:
      case 5:
        size = sizeof(Referee_UI_Drawgrapic_5_t);
        Referee_UI_Drawgrapic_5_t *address_5 =
            (Referee_UI_Drawgrapic_5_t *)send_data;
        address_5->header.sof = REF_HEADER_SOF;
        address_5->header.data_length =
            sizeof(UI_Drawgrapic_5_t) + sizeof(Referee_Interactive_Header_t);
        address_5->header.crc8 =
            CRC8_Calc((const uint8_t *)&(address_5->header),
                      sizeof(Referee_Header_t) - sizeof(uint8_t), CRC8_INIT);
        address_5->cmd_id = REF_CMD_ID_INTER_STUDENT;
        Referee_SetHeader(&(address_5->IA_header), REF_STDNT_CMD_ID_UI_DRAW5,
                          ref->robot_status.robot_id);
        Referee_MoveData(&(ui->grapic), &(address_5->data.grapic),
                         sizeof(UI_Drawgrapic_5_t));
        address_5->crc16 = CRC16_Calc(
            (const uint8_t *)address_5,
            sizeof(Referee_UI_Drawgrapic_5_t) - sizeof(uint16_t), CRC16_INIT);
        break;
      case 6:
      case 7:
        size = sizeof(Referee_UI_Drawgrapic_7_t);
        Referee_UI_Drawgrapic_7_t *address_7 =
            (Referee_UI_Drawgrapic_7_t *)send_data;
        address_7->header.sof = REF_HEADER_SOF;
        address_7->header.data_length =
            sizeof(UI_Drawgrapic_7_t) + sizeof(Referee_Interactive_Header_t);
        address_7->header.crc8 =
            CRC8_Calc((const uint8_t *)&(address_7->header),
                      sizeof(Referee_Header_t) - sizeof(uint8_t), CRC8_INIT);
        address_7->cmd_id = REF_CMD_ID_INTER_STUDENT;
        Referee_SetHeader(&(address_7->IA_header), REF_STDNT_CMD_ID_UI_DRAW7,
                          ref->robot_status.robot_id);
        Referee_MoveData(&(ui->grapic), &(address_7->data.grapic),
                         sizeof(UI_Drawgrapic_7_t));
        address_7->crc16 = CRC16_Calc(
            (const uint8_t *)address_7,
            sizeof(Referee_UI_Drawgrapic_7_t) - sizeof(uint16_t), CRC16_INIT);
        break;
      default:
        return DEVICE_ERR;
    }
    if (Referee_StartSend(send_data, size) == HAL_OK) {
      ui->grapic_counter = 0;
      return DEVICE_OK;
    } else
      return DEVICE_ERR;
  } else if (ui->character_counter != 0) {
    if (ui->character_counter < 0 ||
        ui->character_counter > REF_UI_MAX_STRING_NUM)
      return DEVICE_ERR;
    Referee_UI_Drawcharacter_t *address =
        (Referee_UI_Drawcharacter_t *)send_data;
    address->header.sof = REF_HEADER_SOF;
    address->header.data_length =
        sizeof(UI_Drawcharacter_t) + sizeof(Referee_Interactive_Header_t);
    address->header.crc8 =
        CRC8_Calc((const uint8_t *)&(address->header),
                  sizeof(Referee_Header_t) - sizeof(uint8_t), CRC8_INIT);
    address->cmd_id = REF_CMD_ID_INTER_STUDENT;
    Referee_SetHeader(&(address->IA_header), REF_STDNT_CMD_ID_UI_STR,
                      ref->robot_status.robot_id);
    Referee_MoveData(&(ui->character_data[--ui->character_counter]),
                     &(address->data.grapic), sizeof(UI_Drawcharacter_t));
    address->crc16 = CRC16_Calc(
        (const uint8_t *)address,
        sizeof(Referee_UI_Drawcharacter_t) - sizeof(uint16_t), CRC16_INIT);
    size = sizeof(Referee_UI_Drawcharacter_t);

    Referee_StartSend(send_data, size);
    return DEVICE_OK;
  } else if (ui->del_counter != 0) {
    if (ui->del_counter < 0 || ui->del_counter > REF_UI_MAX_STRING_NUM)
      return DEVICE_ERR;
    Referee_UI_Del_t *address = (Referee_UI_Del_t *)send_data;
    address->header.sof = REF_HEADER_SOF;
    address->header.data_length =
        sizeof(UI_Del_t) + sizeof(Referee_Interactive_Header_t);
    address->header.crc8 =
        CRC8_Calc((const uint8_t *)&(address->header),
                  sizeof(Referee_Header_t) - sizeof(uint8_t), CRC8_INIT);
    address->cmd_id = REF_CMD_ID_INTER_STUDENT;
    Referee_SetHeader(&(address->IA_header), REF_STDNT_CMD_ID_UI_DEL,
                      ref->robot_status.robot_id);
    Referee_MoveData(&(ui->del[--ui->del_counter]), &(address->data),
                     sizeof(UI_Del_t));
    address->crc16 =
        CRC16_Calc((const uint8_t *)address,
                   sizeof(Referee_UI_Del_t) - sizeof(uint16_t), CRC16_INIT);
    size = sizeof(Referee_UI_Del_t);

    Referee_StartSend(send_data, size);
    return DEVICE_OK;
  } else
    return DEVICE_ERR_NULL;
}

UI_Ele_t *Referee_GetGrapicAdd(Referee_UI_t *ref_ui) {
  if (ref_ui->grapic_counter >= REF_UI_MAX_GRAPIC_NUM ||
      ref_ui->grapic_counter < 0)
    return NULL;
  else
    return &(ref_ui->grapic[ref_ui->grapic_counter++]);
}

UI_Drawcharacter_t *Referee_GetCharacterAdd(Referee_UI_t *ref_ui) {
  if (ref_ui->character_counter >= REF_UI_MAX_STRING_NUM ||
      ref_ui->character_counter < 0)
    return NULL;
  else
    return &(ref_ui->character_data[ref_ui->character_counter++]);
}

UI_Del_t *Referee_GetDelAdd(Referee_UI_t *ref_ui) {
  if (ref_ui->del_counter >= REF_UI_MAX_DEL_NUM || ref_ui->del_counter < 0)
    return NULL;
  else
    return &(ref_ui->del[ref_ui->del_counter++]);
}

uint8_t Referee_PraseCmd(Referee_UI_t *ref_ui, CMD_UI_t cmd) {
  switch (cmd) {
    /* Demo */
    case CMD_UI_NOTHING:
      /* 字符 */
      UI_DrawCharacter(Referee_GetCharacterAdd(ref_ui), "0",
                       UI_GRAPIC_OPERATION_ADD, UI_GRAPIC_LAYER_AUTOAIM,
                       RED_BLUE, UI_DEFAULT_WIDTH, 100, 100, 200, 200, "Demo");
      /* 直线 */
      UI_DrawLine(Referee_GetGrapicAdd(ref_ui), "2", UI_GRAPIC_OPERATION_ADD,
                  UI_GRAPIC_LAYER_AUTOAIM, RED_BLUE, UI_DEFAULT_WIDTH, 960, 540,
                  960, 240);
      /* 圆形 */
      UI_DrawCycle(Referee_GetGrapicAdd(ref_ui), "1", UI_GRAPIC_OPERATION_ADD,
                   UI_GRAPIC_LAYER_AUTOAIM, RED_BLUE, UI_DEFAULT_WIDTH, 900,
                   500, 10);
      /* 删除 */
      UI_DelLayer(Referee_GetDelAdd(ref_ui), UI_DEL_OPERATION_DEL,
                  UI_GRAPIC_LAYER_AUTOAIM);
      break;

    default:
      return -1;
  }
  return 0;
}

uint8_t Referee_PackCap(Referee_ForCap_t *cap, const Referee_t *ref) {
  cap->chassis_power_limit = ref->robot_status.chassis_power_limit;
  cap->chassis_pwr_buff = ref->power_heat.chassis_pwr_buff;
  cap->chassis_watt = ref->power_heat.chassis_watt;
  cap->ref_status = ref->ref_status;
  return 0;
}

uint8_t Referee_PackAI(Referee_ForAI_t *ai, const Referee_t *ref) {
  ai->ref_status = ref->ref_status;
  return 0;
}

uint8_t Referee_PackChassis(Referee_ForChassis_t *chassis,
                            const Referee_t *ref) {
  chassis->chassis_power_limit = ref->robot_status.chassis_power_limit;
  chassis->chassis_pwr_buff = ref->power_heat.chassis_pwr_buff;
  chassis->ref_status = ref->ref_status;
  return 0;
}

uint8_t Referee_PackShoot(Referee_ForShoot_t *shoot, Referee_t *ref) {
  memcpy((void *)&(shoot->power_heat), (const void *)&(ref->power_heat),
         sizeof(ref->power_heat));
  memcpy((void *)&shoot->robot_status, (const void *)&ref->robot_status,
         sizeof(ref->robot_status));
  shoot->ref_status = ref->ref_status;
  return 0;
}
