/* 
	裁判系统抽象。

*/

/* Includes ------------------------------------------------------------------*/
#include "referee.h"

/* Include BSP相关的头文件 */
#include "bsp_delay.h"
#include "bsp_uart.h"

/* Include Component相关的头文件 */
#include "user_math.h"

/* Private define ------------------------------------------------------------*/
#define REF_HEADER_SOF (0xA5)

/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static Referee_t *gref;
static bool inited = false;

/* Private function  ---------------------------------------------------------*/
static void Referee_RxCpltCallback(void) {
	osThreadFlagsSet(gref->thread_alert, REFEREE_SIGNAL_RAW_REDY);
}


/* Exported functions --------------------------------------------------------*/
int8_t Referee_Init(Referee_t *ref, osThreadId_t thread_alert) {
	if (ref == NULL)
		return -1;
	
	if (inited)
		return -1;
	
	ref->thread_alert = thread_alert;
	
	BSP_UART_RegisterCallback(BSP_UART_REF, BSP_UART_RX_COMPLETE_CB, Referee_RxCpltCallback);
	
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
	// TODO
	return 0;
}

int8_t Referee_StartReceivingHeader(Referee_t *ref) {
	return BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->header), sizeof(Referee_FrameHeader_t));
}

int8_t Referee_StartReceivingCMDID(Referee_t *ref) {
	return BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->cmd_id), sizeof(Referee_CMDID_t));
}

int8_t Referee_StartReceivingData(Referee_t *ref) {
	switch (ref->cmd_id) {
		case REF_CMD_ID_GAME_STATUS:
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->game_status), sizeof(Referee_GameStatus_t));
			break;
		case REF_CMD_ID_GAME_RESULT:
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->game_result), sizeof(Referee_GameResult_t));
			break;
		case REF_CMD_ID_GAME_ROBOT_HP:	
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->game_robot_hp), sizeof(Referee_GameRobotHP_t));
			break;
		case REF_CMD_ID_DART_STATUS:
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->dart_status), sizeof(Referee_DartStatus_t));
			break;
		case REF_CMD_ID_ICRA_ZONE_STATUS:
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->icra_zone), sizeof(Referee_ICRAZoneStatus_t));
			break;
		case REF_CMD_ID_FIELD_EVENTS:
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->field_event), sizeof(Referee_FieldEvents_t));
			break;
		case REF_CMD_ID_SUPPLY_ACTION:
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->supply_action), sizeof(Referee_SupplyAction_t));
			break;
		case REF_CMD_ID_WARNING:
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->warning), sizeof(Referee_Warning_t));
			break;
		case REF_CMD_ID_DART_COUNTDOWN:
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->dart_countdown), sizeof(Referee_DartCountdown_t));
			break;
		case REF_CMD_ID_ROBOT_STATUS:
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->robot_status), sizeof(Referee_RobotStatus_t));
			break;
		case REF_CMD_ID_POWER_HEAT_DATA:
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->power_heat), sizeof(Referee_PowerHeat_t));
			break;
		case REF_CMD_ID_ROBOT_POS:
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->robot_pos), sizeof(Referee_RobotPos_t));
			break;
		case REF_CMD_ID_ROBOT_BUFF:
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->robot_buff), sizeof(Referee_RobotBuff_t));
			break;
		case REF_CMD_ID_DRONE_ENERGY:
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->drone_energy), sizeof(Referee_DroneEnergy_t));
			break;
		case REF_CMD_ID_ROBOT_DMG:
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->robot_danage), sizeof(Referee_RobotDamage_t));
			break;
		case REF_CMD_ID_SHOOT_DATA:
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->shoot_data), sizeof(Referee_ShootData_t));
			break;
		case REF_CMD_ID_BULLET_REMAINING:
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->bullet_remain), sizeof(Referee_BulletRemain_t));
			break;
		case REF_CMD_ID_RFID:
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->rfid), sizeof(Referee_RFID_t));
			break;
		case REF_CMD_ID_DART_CLIENT:
			BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->dart_client), sizeof(Referee_DartClient_t));
			break;
		default:
			return -1;
	}
	return 0;
}

int8_t Referee_StartReceivingTail(Referee_t *ref) {
	return BSP_UART_ReceiveDMA(BSP_UART_REF, (uint8_t*)&(ref->tail), sizeof(uint16_t));
}

int8_t Referee_CheckHeader(Referee_t *ref) {
	if (ref->header.sof != REF_HEADER_SOF)
		return -1;
	
	// TODO: verify CRC8.
	return 0;
}
