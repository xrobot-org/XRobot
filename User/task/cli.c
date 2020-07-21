/* 
	运行命令行交互界面（Command Line Interface）。
	TODO:命令行作为所有程序的入口，开启其他程序。动态的设定开发板对应的类型。
	实现换板子不用重新烧写程序。
	开机倒计时选择启动型号，配合flash选择默认，保存上次选择结果。
	新建config文件夹，保存各种机器人型号的config。
*/

/* Includes ------------------------------------------------------------------*/
#include "task\user_task.h"

#include <stdio.h>
#include <stdbool.h>

#include "bsp\usb.h"
#include "bsp\flash.h"

#include "FreeRTOS.h"
#include "task.h"
#include "component\FreeRTOS_CLI.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define MAX_INPUT_LENGTH	64

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
Task_Param_t task_param; //TODO: Add static when release

static const char* const CLI_WELCOME_MESSAGE = 
	"\r\n"
	"  ______         __           _______               __              \r\n"
	" |   __ \\.-----.|  |--.-----.|   |   |.---.-.-----.|  |_.-----.----.\r\n"
	" |      <|  _  ||  _  |  _  ||       ||  _  |__ --||   _|  -__|   _|\r\n"
	" |___|__||_____||_____|_____||__|_|__||___._|_____||____|_____|__|  \r\n"
	"           Q I N G D A O  U N I V E R S I T Y    2 0 2 0            \r\n"
	" -------------------------------------------------------------------\r\n"
	" Firmware Version: 0.0.1                                            \r\n"
	" -------------------------------------------------------------------\r\n"
	" FreeRTOS CLI. Type 'help' to view a list of registered commands.   \r\n"
	"\r\n";

/* experiment */
static BaseType_t EndianCommand(char *out_buffer, size_t len, const char *command_string) {
	(void)command_string;
	
	if (out_buffer == NULL)
		return pdFALSE;
	
	uint8_t list[2] = {0x11, 0x22};
    uint16_t force_convert = ((uint16_t*)list)[0];
    uint16_t assembled = (list[0] | (list[1] << 8)) & 0xFFFF;
	
	len -= 1;
	static uint8_t stage = 0;
	switch (stage) {
		case 0:
			snprintf(out_buffer, len, "a[2] = {0x11, 0x22}\r\n");
			stage ++;
			return pdPASS;
		case 1:
			snprintf(out_buffer, len, "Force convert to uint16 list, we got: 0x%x\r\n", force_convert);
			stage ++;
			return pdPASS;
		case 2:
			snprintf(out_buffer, len, "Manually assemble a[1], a[0], we got: 0x%x\r\n", assembled);
			stage ++;
			return pdPASS;
		case 3:
			if (force_convert == assembled)
				snprintf(out_buffer, len, "Small endian\r\n");
			else
				snprintf(out_buffer, len, "Big endian\r\n");
			stage ++;
			return pdPASS;
		default:
			snprintf(out_buffer, len, "\r\n");
			stage = 0;
			return pdFALSE;
	}
}

static const CLI_Command_Definition_t endian = {
	"endian",
	"\r\nendian:\r\n Endian experiment.\r\n\r\n",
	EndianCommand,
	0, 
};

/* debug */
static BaseType_t StatsCommand(char *out_buffer, size_t len, const char *command_string) {
	static const char *const task_list_header = 
		"Task list\r\n"
		"Task          State  Priority  Stack	#\r\n"
		"************************************************\r\n";
	
	static const char *const run_time_header = 
		"Run time stats\r\n"
		"Task            Abs Time      % Time\r\n"
		"****************************************\r\n";

	static const char *const heap_header = 
		"Heap stats\r\n"
		"total(B)	free(B)	used(B)\r\n"
		"*******************************\r\n";
	
	(void)command_string;
	if (out_buffer == NULL)
		return pdFALSE;
	
	HeapStats_t heap_stats;
	
	len -= 1;
	static uint8_t stage = 0;
	switch (stage) {
		case 0:
			strncpy(out_buffer, task_list_header, len);
			stage ++;
			return pdPASS;
		case 1:
			vTaskList(out_buffer);
			stage ++;
			return pdPASS;
		case 2:
			strncat(out_buffer, run_time_header, len);
			stage ++;
			return pdPASS;
		case 3:
			vTaskGetRunTimeStats(out_buffer);
			stage ++;
			return pdPASS;
		case 4:
			strncat(out_buffer, heap_header, len);
			stage ++;
			return pdPASS;
		case 5:	
			vPortGetHeapStats(&heap_stats);
			snprintf(out_buffer, len, "%d\t\t%d\t%d\r\n", configTOTAL_HEAP_SIZE, heap_stats.xAvailableHeapSpaceInBytes,configTOTAL_HEAP_SIZE - heap_stats.xAvailableHeapSpaceInBytes);
			stage ++;
			return pdPASS;
		default:
			snprintf(out_buffer, len, "\r\n");
			stage = 0;
			return pdFALSE;
	}
}

static const CLI_Command_Definition_t stats = {
	"stats",
	"\r\nstats:\r\n Displays a table showing the state of FreeRTOS\r\n\r\n",
	StatsCommand,
	0, 
};

static BaseType_t SetModelCommand(char *out_buffer, size_t len, const char *command_string) {
	const char *param;
	BaseType_t param_len;
	
	(void)command_string;
	
	if (out_buffer == NULL)
		return pdFALSE;
	
	param = FreeRTOS_CLIGetParameter(command_string, 1, &param_len);
	
	if (param == NULL)
		return pdFALSE;
	
	len -= 1;
	static uint8_t stage = 0;
	switch (stage) {
		case 0:
			snprintf(out_buffer, len, "Set robot model to: ");
			stage ++;
			return pdPASS;
		case 1:
			switch (*param) {
				case 'I':
					snprintf(out_buffer, len, "Infantry.");
					break;
				case 'H':
					snprintf(out_buffer, len, "Hero.");
					break;
				case 'E':
					snprintf(out_buffer, len, "Engineer.");
					break;
				case 'D':
					snprintf(out_buffer, len, "Drone.");
					break;
				case 'S':
					snprintf(out_buffer, len, "Sentry.");
					break;
				default:
					snprintf(out_buffer, len, "Unknow model. Check help for avaliable options.");
			}
			stage ++;
			return pdPASS;
		default:
			snprintf(out_buffer, len, "\r\n");
			stage = 0;
			return pdFALSE;
	}
}

static const CLI_Command_Definition_t set_model = {
	"set-model",
	"\r\nset-model <model>:\r\n Set robot model. Expext:I[nfantry], H[ero], E[ngineer], D[rone] and S[entry]\r\n\r\n",
	SetModelCommand,
	1,
};

static char input[MAX_INPUT_LENGTH];

/* Private function ----------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_CLI(void *argument) {
	(void)argument;
	
	uint16_t index = 0;
	BaseType_t processing = 0;
	char rx_char;
	char *output = FreeRTOS_CLIGetOutputBuffer();
	
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_CLI);
	
	/* Register all the commands. */
	FreeRTOS_CLIRegisterCommand(&endian);
	FreeRTOS_CLIRegisterCommand(&stats);
	FreeRTOS_CLIRegisterCommand(&set_model);
	
	/* Init robot part. */
	task_param.config = Config_GetRobotDefalult();
	
	task_param.thread.cli = osThreadGetId();
	
	osKernelLock();
	task_param.thread.command		= osThreadNew(Task_Command,		&task_param, &command_attr);
	task_param.thread.ctrl_chassis	= osThreadNew(Task_CtrlChassis,	&task_param, &ctrl_chassis_attr);
	task_param.thread.ctrl_gimbal	= osThreadNew(Task_CtrlGimbal,	&task_param, &ctrl_gimbal_attr);
	task_param.thread.ctrl_shoot	= osThreadNew(Task_CtrlShoot,	&task_param, &ctrl_shoot_attr);
	task_param.thread.info			= osThreadNew(Task_Info,		&task_param, &info_attr);
	task_param.thread.monitor		= osThreadNew(Task_Monitor,		&task_param, &monitor_attr);
	task_param.thread.pos_esti		= osThreadNew(Task_PosEsti,		&task_param, &pos_esti_attr);
	task_param.thread.referee		= osThreadNew(Task_Referee,		&task_param, &referee_attr);
	osKernelUnlock();
	
	/* Command Line Interface part. */
	BSP_USB_Printf("Please press ENTER to activate this console.\r\n");
	while(1) {
		BSP_USB_ReadyReceive(osThreadGetId());
		osThreadFlagsWait(BSP_USB_SIGNAL_BUF_RECV, osFlagsWaitAll, osWaitForever);

		rx_char = BSP_USB_ReadChar();
		BSP_USB_Printf("%c", rx_char);
		
		if (rx_char == '\n' || rx_char == '\r') {
			break;
		}
	}
	
	BSP_USB_Printf(CLI_WELCOME_MESSAGE);
	
	BSP_USB_Printf("rm>");
	
	while(1) {
#ifdef DEBUG
		task_param.stack_water_mark.cli = uxTaskGetStackHighWaterMark(NULL);
#endif
		/* Task body */
		
		/* Wait for input. */
		BSP_USB_ReadyReceive(osThreadGetId());
		osThreadFlagsWait(BSP_USB_SIGNAL_BUF_RECV, osFlagsWaitAll, osWaitForever);
		
		rx_char = BSP_USB_ReadChar();
		
		if (rx_char <= 126 && rx_char >= 32) {
			if (index < MAX_INPUT_LENGTH) {
				BSP_USB_Printf("%c", rx_char);
				input[index] = rx_char;
				index++;
			}
		} else {
			if (rx_char == '\n' || rx_char == '\r') {\
				BSP_USB_Printf("\r\n");
				if (index > 0) {
					do {
						processing = FreeRTOS_CLIProcessCommand(input, output, configCOMMAND_INT_MAX_OUTPUT_SIZE);
						BSP_USB_Printf(output);
					} while(processing != pdFALSE);
					index = 0;
					memset(input, 0x00, MAX_INPUT_LENGTH);
				}
				BSP_USB_Printf("rm>");
			} else if (rx_char == '\b' || rx_char == 0x7Fu) {
				if (index > 0) {
					BSP_USB_Printf("%c", rx_char);
					index--;
					input[index] = '\0';
				}
			}
		}
	}
}
