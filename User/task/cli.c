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
	
/* Private function ----------------------------------------------------------*/
static BaseType_t EndianCommand(char *out_buffer, size_t len, const char *command_string) {
	(void)command_string;
	(void)len;
	configASSERT(out_buffer);
	
    int i = 0;
	uint8_t list[2] = {0x11, 0x22};
    uint16_t force_convert = ((uint16_t*)list)[0];
	
	i += sprintf(out_buffer + i, "a[2] = {0x11, 0x22}\r\n");
	i += sprintf(out_buffer + i, "Force convert to uint16 list, we got: 0x%x\r\n", force_convert);

    uint16_t assembled = list[0] | (list[1] << 8);
    i += sprintf(out_buffer + i, "Manually assemble a[1], a[0], we got: 0x%x\r\n", assembled);
	
	if (force_convert == assembled)
		i += sprintf(out_buffer + i, "Small endian\r\n");
	else
		i += sprintf(out_buffer + i, "Big endian\r\n");

	strcat(out_buffer, "\r\n");
	return pdFALSE;
}

static const CLI_Command_Definition_t endian = {
	"endian",
	"\r\nendian:\r\n Endian experiment.\r\n\r\n",
	EndianCommand,
	0, 
};


static BaseType_t TaskStatsCommand(char *out_buffer, size_t len, const char *command_string) {
	const char *const header = 
		"Task          State  Priority  Stack	#\r\n"
		"************************************************\r\n";
	(void)command_string;
	(void)len;
	configASSERT(out_buffer);

	strcpy(out_buffer, header);
	vTaskList(out_buffer + strlen(header));

	return pdFALSE;
}

static const CLI_Command_Definition_t task_stats = {
	"task-stats",
	"\r\ntask-stats:\r\n Displays a table showing the state of each FreeRTOS task\r\n\r\n",
	TaskStatsCommand,
	0, 
};

static BaseType_t HeapStatsCommand(char *out_buffer, size_t len, const char *command_string) {
	const char *const header = 
		"total(B)	free(B)	used(B)\r\n"
		"*******************************\r\n";
	(void)command_string;
	(void)len;
	configASSERT(out_buffer);
	HeapStats_t heap_stats;
	
	strcpy(out_buffer, header);
	vPortGetHeapStats(&heap_stats);
	sprintf(out_buffer + strlen(header), "%d\t\t%d\t%d\r\n", configTOTAL_HEAP_SIZE, heap_stats.xAvailableHeapSpaceInBytes,configTOTAL_HEAP_SIZE - heap_stats.xAvailableHeapSpaceInBytes);

	return pdFALSE;
}

static const CLI_Command_Definition_t heap_stats = {
	"heap-stats",
	"\r\nheap-stats:\r\n Displays a table showing the state of memory\r\n\r\n",
	HeapStatsCommand,
	0, 
};

static BaseType_t TempCommand(char *out_buffer, size_t len, const char *command_string) {
	const char *const header = 
		"CPU(C)\r\n"
		"*******************************\r\n";
	(void)command_string;
	(void)len;
	configASSERT(out_buffer);
	
	strcpy(out_buffer, header);
	sprintf(out_buffer + strlen(header), "%.2f\r\n", -1.f); //TODO

	return pdFALSE;
}

static const CLI_Command_Definition_t temp_stats = {
	"temp-stats",
	"\r\nheap-stats:\r\n Displays a table showing the state of temperature\r\n\r\n",
	TempCommand,
	0, 
};

static BaseType_t RunTimeStatsCommand(char *out_buffer, size_t len, const char *command_string) {
	const char * const header = 
		"Task            Abs Time      % Time\r\n"
		"****************************************\r\n";

	(void)command_string;
	(void)len;
	configASSERT(out_buffer);

	strcpy(out_buffer, header);
	vTaskGetRunTimeStats(out_buffer + strlen(header));

	return pdFALSE;
}

static const CLI_Command_Definition_t run_time_stats = {
	"run-time-stats",
	"\r\nrun-time-stats:\r\n Displays a table showing how much processing time each FreeRTOS task has used\r\n\r\n",
	RunTimeStatsCommand,
	0,
};

static BaseType_t SetModelCommand(char *out_buffer, size_t len, const char *command_string) {
	const char *param;
	BaseType_t param_len;
	
	configASSERT(out_buffer);
	
	param = FreeRTOS_CLIGetParameter(command_string, 1, &param_len);
	configASSERT(param);
	
	memset(out_buffer, 0x00, len);
	sprintf(out_buffer, "Set robot model to: ");
	switch (*param) {
		case 'I':
			strcat(out_buffer, "Infantry.");
			break;
		case 'H':
			strcat(out_buffer, "Hero.");
			break;
		case 'E':
			strcat(out_buffer, "Engineer.");
			break;
		case 'D':
			strcat(out_buffer, "Drone.");
			break;
		case 'S':
			strcat(out_buffer, "Sentry.");
			break;
		default:
			strcat(out_buffer, "Unknow model. Check help for avaliable options.");
	}
	strcat(out_buffer, "\r\n");
	return pdFALSE;
}

static const CLI_Command_Definition_t set_model = {
	"set-model",
	"\r\nset-model <model>:\r\n Set robot model. Expext:I[nfantry], H[ero], E[ngineer], D[rone] and S[entry]\r\n\r\n",
	SetModelCommand,
	1,
};

/* Exported functions --------------------------------------------------------*/
void Task_CLI(void *argument) {
	//Task_Param_t *task_param = (Task_Param_t*)argument;
	
	char rx_char;
	uint16_t index = 0;
	BaseType_t processing = 0;
	char *output = FreeRTOS_CLIGetOutputBuffer();
	static char input[MAX_INPUT_LENGTH];
	
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_CLI);
	
	/* Register all the commands. */
	FreeRTOS_CLIRegisterCommand(&task_stats);
	FreeRTOS_CLIRegisterCommand(&run_time_stats);
	FreeRTOS_CLIRegisterCommand(&set_model);
	FreeRTOS_CLIRegisterCommand(&endian);
	FreeRTOS_CLIRegisterCommand(&heap_stats);
	FreeRTOS_CLIRegisterCommand(&temp_stats);
	
	/* Save CPU power when CLI not used. */
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
