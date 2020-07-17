/* 
	裁判系统抽象。

*/

/* Includes ------------------------------------------------------------------*/
#include "referee.h"

#include "bsp\delay.h"
#include "bsp\uart.h"

#include "component\crc8.h"
#include "component\crc16.h"
#include "component\user_math.h"

/* Private define ------------------------------------------------------------*/
#define REF_HEADER_SOF (0xA5)
#define REF_LEN_RX_BUFF (0xFF)

/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static Referee_t *gref;
static bool inited = false;
static uint8_t rxbuf[REF_LEN_RX_BUFF];

/* Private function  ---------------------------------------------------------*/
static void Referee_RxCpltCallback(void) {
	osThreadFlagsSet(gref->thread_alert, REFEREE_SIGNAL_RAW_REDY);
}

static void Referee_IdleLineCallback(void) {
	HAL_UART_AbortReceive_IT(BSP_UART_GetHandle(BSP_UART_REF));
}

static void Referee_AbortRxCpltCallback(void) {
	osThreadFlagsSet(gref->thread_alert, REFEREE_SIGNAL_RAW_REDY);
}

/* Exported functions --------------------------------------------------------*/
int8_t Referee_Init(Referee_t *ref, osThreadId_t thread_alert) {
	if (ref == NULL)
		return -1;
	
	if (inited)
		return -1;
	
	ref->thread_alert = thread_alert;
	
	BSP_UART_RegisterCallback(BSP_UART_REF, BSP_UART_RX_CPLT_CB, Referee_RxCpltCallback);
	BSP_UART_RegisterCallback(BSP_UART_REF, BSP_UART_ABORT_RX_CPLT_CB, Referee_AbortRxCpltCallback);
	BSP_UART_RegisterCallback(BSP_UART_REF, BSP_UART_IDLE_LINE_CB, Referee_IdleLineCallback);
	
	__HAL_UART_ENABLE_IT(BSP_UART_GetHandle(BSP_UART_REF), UART_IT_IDLE);
	
	gref = ref;
	inited = true;
	return 0;
}

Referee_t *Referee_GetDevice(void) {
	if (inited)
		return gref;
	
	return NULL;
}

int8_t Referee_Restart(void) {
	__HAL_UART_DISABLE(BSP_UART_GetHandle(BSP_UART_DR16));
	__HAL_UART_ENABLE(BSP_UART_GetHandle(BSP_UART_DR16));
	return 0;
}

int8_t Referee_StartReceiving(Referee_t *ref) {
	return HAL_UART_Receive_DMA(BSP_UART_GetHandle(BSP_UART_REF), rxbuf, sizeof(REF_LEN_RX_BUFF));
}

int8_t Referee_Parse(Referee_t *ref) {
	uint8_t data_length = REF_LEN_RX_BUFF - __HAL_DMA_GET_COUNTER(BSP_UART_GetHandle(BSP_UART_REF)->hdmarx);
	
	uint8_t index = 0;
	
	Referee_Header_t *header = (Referee_Header_t*)(rxbuf + index);
	index += sizeof(Referee_Header_t);
	if (index >= data_length)
		return -1;
	
	if (CRC8_Verify((uint8_t*)header, sizeof(Referee_Header_t)))
		return -1;
	
	if (header->sof != REF_HEADER_SOF)
		return -1;
	
	Referee_CMDID_t *cmd_id = (Referee_CMDID_t*)(rxbuf + index);
	index += sizeof(Referee_CMDID_t);
	if (index >= data_length)
		return -1;
	
	void *target = (rxbuf + index);
	void *origin;
	size_t size;
	
	switch (*cmd_id) {
		case REF_CMD_ID_GAME_STATUS:
			origin = &(ref->game_status);
			size = sizeof(Referee_GameStatus_t);
			break;
		case REF_CMD_ID_GAME_RESULT:
			origin = &(ref->game_result); 
			size = sizeof(Referee_GameResult_t);
			break;
		case REF_CMD_ID_GAME_ROBOT_HP:	
			origin = &(ref->game_robot_hp);
			size = sizeof(Referee_GameRobotHP_t);
			break;
		case REF_CMD_ID_DART_STATUS:
			origin = &(ref->dart_status);
			size = sizeof(Referee_DartStatus_t);
			break;
		case REF_CMD_ID_ICRA_ZONE_STATUS:
			origin = &(ref->icra_zone);
			size = sizeof(Referee_ICRAZoneStatus_t);
			break;
		case REF_CMD_ID_FIELD_EVENTS:
			origin = &(ref->field_event);
			size = sizeof(Referee_FieldEvents_t);
			break;
		case REF_CMD_ID_SUPPLY_ACTION:
			origin = &(ref->supply_action);
			size = sizeof(Referee_SupplyAction_t);
			break;
		case REF_CMD_ID_WARNING:
			origin = &(ref->warning);
			size = sizeof(Referee_Warning_t);
			break;
		case REF_CMD_ID_DART_COUNTDOWN:
			origin = &(ref->dart_countdown);
			size = sizeof(Referee_DartCountdown_t);
			break;
		case REF_CMD_ID_ROBOT_STATUS:
			origin = &(ref->robot_status);
			size = sizeof(Referee_RobotStatus_t);
			break;
		case REF_CMD_ID_POWER_HEAT_DATA:
			origin = &(ref->power_heat);
			size = sizeof(Referee_PowerHeat_t);
			break;
		case REF_CMD_ID_ROBOT_POS:
			origin = &(ref->robot_pos);
			size = sizeof(Referee_RobotPos_t);
			break;
		case REF_CMD_ID_ROBOT_BUFF:
			origin = &(ref->robot_buff);
			size = sizeof(Referee_RobotBuff_t);
			break;
		case REF_CMD_ID_DRONE_ENERGY:
			origin = &(ref->drone_energy);
			size = sizeof(Referee_DroneEnergy_t);
			break;
		case REF_CMD_ID_ROBOT_DMG:
			origin = &(ref->robot_danage);
			size = sizeof(Referee_RobotDamage_t);
			break;
		case REF_CMD_ID_SHOOT_DATA:
			origin = &(ref->shoot_data);
			size = sizeof(Referee_ShootData_t);
			break;
		case REF_CMD_ID_BULLET_REMAINING:
			origin = &(ref->bullet_remain);
			size = sizeof(Referee_BulletRemain_t);
			break;
		case REF_CMD_ID_RFID:
			origin = &(ref->rfid);
			size = sizeof(Referee_RFID_t);
			break;
		case REF_CMD_ID_DART_CLIENT:
			origin = &(ref->dart_client);
			size = sizeof(Referee_DartClient_t);
			break;
		default:
			return -1;
	}
	index += size;
	if (index >= data_length)
		return -1;
	
	index += sizeof(Referee_Tail_t);
	if (index != data_length)
		return -1;
	
	if (CRC16_Verify((uint8_t*)header, sizeof(Referee_Header_t)))
		memcpy(target, origin, size);
	else
		return -1;
	
	return 0;
}
