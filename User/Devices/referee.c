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
typedef __packed struct {
	uint8_t sof;
	uint16_t data_length;
	uint8_t seq;
	uint8_t crc8;
} Referee_FrameHeader_t;

typedef enum {
	REF_CMD_ID_GAME_STATUS			= 0x0001,
	REF_CMD_ID_GAME_RESULT			= 0x0002,
	REF_CMD_ID_GAME_ROBOT_HP		= 0x0003,
	REF_CMD_ID_DART_STATUS			= 0x0004,
	REF_CMD_ID_ICRA_ZONE_STATUS		= 0x0005,
	REF_CMD_ID_FIELD_EVENTS			= 0x0101,
	REF_CMD_ID_SUPPLY_ACTION		= 0x0102,
	REF_CMD_ID_WARNING				= 0x0104,
	REF_CMD_ID_DART_COUNTDOWN		= 0x0105,
	REF_CMD_ID_ROBOT_STATUS			= 0x0201,
	REF_CMD_ID_POWER_HEAT_DATA		= 0x0202,
	REF_CMD_ID_ROBOT_POS			= 0x0203,
	REF_CMD_ID_ROBOT_BUFF			= 0x0204,
	REF_CMD_ID_DRONE_ENERGY			= 0x0205,
	REF_CMD_ID_ROBOT_DMG			= 0x0206,
	REF_CMD_ID_SHOOT_DATA			= 0x0207,
	REF_CMD_ID_BULLET_REMAINING		= 0x0208,
	REF_CMD_ID_RFID					= 0x0209,
	REF_CMD_ID_DART_CLIENT			= 0x020A,
	REF_CMD_ID_INTER_STUDENT		= 0x0301,
} Referee_CMDID_t;

/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
static void Referee_RxCpltCallback(void) {
	
}

/* Exported functions --------------------------------------------------------*/
int8_t Referee_Update(Referee_t *ref, const uint8_t *buf) {
	// TODO: verify CRC.
	Referee_FrameHeader_t *header = (Referee_FrameHeader_t*)buf;
	buf += sizeof(Referee_FrameHeader_t);
	
	if(header->sof != REF_HEADER_SOF)
		return -1;
	
	//if(header->crc8 != )
	//	return -1;
	
	Referee_CMDID_t *cmd_id = (Referee_CMDID_t*)buf; 
	buf += sizeof(Referee_CMDID_t);
	
	switch (*cmd_id) {
		case REF_CMD_ID_GAME_STATUS:
			memcpy(&(ref->game_status), buf, sizeof(Referee_GameStatus_t));
			buf += sizeof(Referee_CMDID_t);
			break;
		case REF_CMD_ID_GAME_RESULT:
			memcpy(&(ref->game_result), buf, sizeof(Referee_GameResult_t));
			break;
		case REF_CMD_ID_GAME_ROBOT_HP:	
			memcpy(&(ref->game_robot_hp), buf, sizeof(Referee_GameRobotHP_t));
			break;
		case REF_CMD_ID_DART_STATUS:
			memcpy(&(ref->dart_status), buf, sizeof(Referee_DartStatus_t));
			break;
		case REF_CMD_ID_ICRA_ZONE_STATUS:
			memcpy(&(ref->icra_zone), buf, sizeof(Referee_ICRAZoneStatus_t));
			break;
		case REF_CMD_ID_FIELD_EVENTS:
			memcpy(&(ref->field_event), buf, sizeof(Referee_FieldEvents_t));
			break;
		case REF_CMD_ID_SUPPLY_ACTION:
			memcpy(&(ref->supply_action), buf, sizeof(Referee_SupplyAction_t));
			break;
		case REF_CMD_ID_WARNING:
			memcpy(&(ref->warning), buf, sizeof(Referee_Warning_t));
			break;
		case REF_CMD_ID_DART_COUNTDOWN:
			memcpy(&(ref->dart_countdown), buf, sizeof(Referee_DartCountdown_t));
			break;
		case REF_CMD_ID_ROBOT_STATUS:
			memcpy(&(ref->robot_status), buf, sizeof(Referee_RobotStatus_t));
			break;
		case REF_CMD_ID_POWER_HEAT_DATA:
			memcpy(&(ref->power_heat), buf, sizeof(Referee_PowerHeat_t));
			break;
		case REF_CMD_ID_ROBOT_POS:
			memcpy(&(ref->robot_pos), buf, sizeof(Referee_RobotPos_t));
			break;
		case REF_CMD_ID_ROBOT_BUFF:
			memcpy(&(ref->robot_buff), buf, sizeof(Referee_RobotBuff_t));
			break;
		case REF_CMD_ID_DRONE_ENERGY:
			memcpy(&(ref->drone_energy), buf, sizeof(Referee_DroneEnergy_t));
			break;
		case REF_CMD_ID_ROBOT_DMG:
			memcpy(&(ref->robot_danage), buf, sizeof(Referee_RobotDamage_t));
			break;
		case REF_CMD_ID_SHOOT_DATA:
			memcpy(&(ref->shoot_data), buf, sizeof(Referee_ShootData_t));
			break;
		case REF_CMD_ID_BULLET_REMAINING:
			memcpy(&(ref->bullet_remain), buf, sizeof(Referee_BulletRemain_t));
			break;
		case REF_CMD_ID_RFID:
			memcpy(&(ref->rfid), buf, sizeof(Referee_RFID_t));
			break;
		case REF_CMD_ID_DART_CLIENT:
			memcpy(&(ref->dart_client), buf, sizeof(Referee_DartClient_t));
			break;
		default:
			break;
	}
	uint16_t *tail = (uint16_t*)buf;
	(void)tail;
	
	return 0;
}

