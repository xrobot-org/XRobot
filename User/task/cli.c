/*
  命令行交互界面（Command Line Interface）任务

  实现命令行。

  从USB虚拟串口读取数据，结果也打印到USB虚拟串口。
*/

/* Includes ----------------------------------------------------------------- */
#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "bsp\can.h"
#include "bsp\usb.h"
#include "component\FreeRTOS_CLI.h"
#include "task.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
#define MAX_INPUT_LENGTH 64

/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */

static const char *const CLI_WELCOME_MESSAGE =
    "\r\n"
    "  ______         __           _______               __              \r\n"
    " |   __ \\.-----.|  |--.-----.|   |   |.---.-.-----.|  |_.-----.----.\r\n"
    " |      <|  _  ||  _  |  _  ||       ||  _  |__ --||   _|  -__|   _|\r\n"
    " |___|__||_____||_____|_____||__|_|__||___._|_____||____|_____|__|  \r\n"
    "           Q I N G D A O  U N I V E R S I T Y    2 0 2 0            \r\n"
    " -------------------------------------------------------------------\r\n"
    " FreeRTOS CLI. Type 'help' to view a list of registered commands.   \r\n"
    "\r\n";

/* Command示例 */
static BaseType_t Command_Endian(char *out_buffer, size_t len,
                                 const char *command_string) {
  (void)command_string; /* 没用到command_string，消除警告 */

  if (out_buffer == NULL) return pdFALSE;

  /* 任务本身相关的内容 */
  uint8_t list[2] = {0x11, 0x22};
  uint16_t force_convert = ((uint16_t *)list)[0];
  uint16_t assembled = (uint16_t)(list[0] | (list[1] << 8));

  len -= 1;                 /* 字符串后面有\0 */
  static uint8_t stage = 0; /* 有限状态机的状态 */
  switch (stage) {
    case 0:
      /* 每个状态内只允许有一个print相关函数，以保证安全 */
      /* 每个print相关函数必须带有长度限制 */
      snprintf(out_buffer, len, "a[2] = {0x11, 0x22}\r\n");
      stage++;       /* 改变状态机运行状态 */
      return pdPASS; /* 需要继续运行下一状态时返回pdPASS */
    case 1:
      snprintf(out_buffer, len,
               "Force convert to uint16 list, we got: 0x%x\r\n", force_convert);
      stage++;
      return pdPASS;
    case 2:
      snprintf(out_buffer, len,
               "Manually assemble a[1], a[0], we got: 0x%x\r\n", assembled);
      stage++;
      return pdPASS;
    case 3:
      if (force_convert == assembled)
        snprintf(out_buffer, len, "Small endian\r\n");
      else
        snprintf(out_buffer, len, "Big endian\r\n");
      stage++;
      return pdPASS;
    default: /* 结束时状态 */
      snprintf(out_buffer, len, "\r\n");
      stage = 0;      /* 重置有限状态机 */
      return pdFALSE; /* 不需要继续运行下一状态时返回pdFALSE */
  }
}

static BaseType_t Command_Stats(char *out_buffer, size_t len,
                                const char *command_string) {
  static const char *const task_list_header =
      "\r\n"
      "Task list\r\n"
      "Task          State  Priority  Stack	#\r\n"
      "************************************************\r\n";

  static const char *const run_time_header =
      "\r\n"
      "Run time stats\r\n"
      "Task            Abs Time      % Time\r\n"
      "****************************************\r\n";

  static const char *const heap_header =
      "\r\n"
      "Heap stats\r\n"
      "total(B)	free(B)	used(B)\r\n"
      "*******************************\r\n";

  (void)command_string;
  if (out_buffer == NULL) return pdFALSE;

  HeapStats_t heap_stats;

  len -= 1;
  static uint8_t stage = 0;
  switch (stage) {
    case 0:
      strncpy(out_buffer, task_list_header, len);
      stage++;
      return pdPASS;
    case 1:
      vTaskList(out_buffer);
      stage++;
      return pdPASS;
    case 2:
      strncpy(out_buffer, run_time_header, len);
      stage++;
      return pdPASS;
    case 3:
      vTaskGetRunTimeStats(out_buffer);
      stage++;
      return pdPASS;
    case 4:
      strncpy(out_buffer, heap_header, len);
      stage++;
      return pdPASS;
    case 5:
      vPortGetHeapStats(&heap_stats);
      snprintf(out_buffer, len, "%d\t\t%d\t%d\r\n", configTOTAL_HEAP_SIZE,
               heap_stats.xAvailableHeapSpaceInBytes,
               configTOTAL_HEAP_SIZE - heap_stats.xAvailableHeapSpaceInBytes);
      stage++;
      return pdPASS;
    default:
      snprintf(out_buffer, len, "\r\n");
      stage = 0;
      return pdFALSE;
  }
}

static BaseType_t Command_SetModel(char *out_buffer, size_t len,
                                   const char *command_string) {
  const char *param;
  BaseType_t param_len;
  Robot_ID_t id;

  if (out_buffer == NULL) return pdFALSE;

  param = FreeRTOS_CLIGetParameter(command_string, 1, &param_len);

  if (param == NULL) return pdFALSE;

  len -= 1;
  static uint8_t stage = 0;
  switch (stage) {
    case 0:
      snprintf(out_buffer, len, "Set robot model to: ");
      stage = 1;
      return pdPASS;
    case 1:
      Robot_GetRobotID(&id);
      if ((id.model = Robot_GetModelByName(param)) == ROBOT_MODEL_NUM) {
        stage = 2;
        return pdPASS;
      } else {
        snprintf(out_buffer, len, "%s", Robot_GetNameByModel(id.model));
        Robot_SetRobotID(&id);
        stage = 3;
        return pdPASS;
      }
    case 2:
      snprintf(out_buffer, len,
               "Unknow model.\r\nCheck help for avaliable options.");
      stage = 4;
      return pdPASS;
    case 3:
      snprintf(out_buffer, len,
               "\r\nRestart needed for setting to take effect.");
      stage = 4;
      return pdPASS;
    default:
      snprintf(out_buffer, len, "\r\n");
      stage = 0;
      return pdFALSE;
  }
}

static BaseType_t Command_SetPilot(char *out_buffer, size_t len,
                                   const char *command_string) {
  const char *param;
  BaseType_t param_len;
  Robot_ID_t id;

  if (out_buffer == NULL) return pdFALSE;

  param = FreeRTOS_CLIGetParameter(command_string, 1, &param_len);

  if (param == NULL) return pdFALSE;

  len -= 1;
  static uint8_t stage = 0;
  switch (stage) {
    case 0:
      snprintf(out_buffer, len, "Set robot pilot to: ");
      stage = 1;
      return pdPASS;
    case 1:
      Robot_GetRobotID(&id);
      if ((id.pilot = Robot_GetPilotByName(param)) == ROBOT_PILOT_NUM) {
        stage = 2;
        return pdPASS;
      } else {
        snprintf(out_buffer, len, "%s", Robot_GetNameByPilot(id.pilot));
        Robot_SetRobotID(&id);
        stage = 3;
        return pdPASS;
      }
    case 2:
      snprintf(out_buffer, len,
               "Unauthorized pilot.\r\nCheck help for avaliable options.");
      stage = 4;
      return pdPASS;
    case 3:
      snprintf(out_buffer, len,
               "\r\nRestart needed for setting to take effect.");
      stage = 4;
      return pdPASS;
    default:
      snprintf(out_buffer, len, "\r\n");
      stage = 0;
      return pdFALSE;
  }
}

static BaseType_t Command_Error(char *out_buffer, size_t len,
                                const char *command_string) {
  (void)command_string;
  if (out_buffer == NULL) return pdFALSE;

  len -= 1;
  static uint8_t stage = 0;
  switch (stage) {
    case 0:
      snprintf(out_buffer, len, "\r\nError status.");
      stage++;
      return pdPASS;
    case 1:
      // TODO
      stage++;
      return pdPASS;
    default:
      snprintf(out_buffer, len, "\r\n");
      stage = 0;
      return pdFALSE;
  }
}

static BaseType_t Command_MotorIDQuitckSet(char *out_buffer, size_t len,
                                           const char *command_string) {
  (void)command_string;
  if (out_buffer == NULL) return pdFALSE;

  len -= 1;
  static uint8_t stage = 0;
  switch (stage) {
    case 0:
      snprintf(out_buffer, len, "\r\nEnter ID quick set mode.");
      stage++;
      return pdPASS;
    case 1:
      CAN_Motor_QuickIdSetMode();
      snprintf(out_buffer, len, "\r\nDone.");
      stage++;
      return pdPASS;
    default:
      snprintf(out_buffer, len, "\r\n");
      stage = 0;
      return pdFALSE;
  }
}
                                           
static BaseType_t Command_ClearRobotID(char *out_buffer, size_t len,
                                           const char *command_string) {
  (void)command_string;
  if (out_buffer == NULL) return pdFALSE;
  Robot_ID_t id;
  
  len -= 1;
  static uint8_t stage = 0;
  switch (stage) {
    case 0:
      snprintf(out_buffer, len, "\r\nReset Robot ID stored on flash.");
      stage++;
      return pdPASS;
    case 1:
      memset(&id, 0, sizeof(Robot_ID_t));
      Robot_SetRobotID(&id);
      snprintf(out_buffer, len, "\r\nDone.");
      stage++;
      return pdPASS;
    default:
      snprintf(out_buffer, len, "\r\n");
      stage = 0;
      return pdFALSE;
  }
}

/*
static BaseType_t Command_XXX(char *out_buffer, size_t len,
                                           const char *command_string) {
  (void)command_string;
  if (out_buffer == NULL) return pdFALSE;

  len -= 1;
  static uint8_t stage = 0;
  switch (stage) {
    case 0:

      snprintf(out_buffer, len, "\r\nXXX.");


      stage++;
      return pdPASS;
    case 1:
      XXX();
      snprintf(out_buffer, len, "\r\nDone.");


      stage++;
      return pdPASS;

    default:
      snprintf(out_buffer, len, "\r\n");
      stage = 0;
      return pdFALSE;
  }
}
*/

static const CLI_Command_Definition_t command_table[] = {
    {
        "endian",
        "\r\nendian:\r\n Endian experiment.\r\n\r\n",
        Command_Endian,
        0,
    },
    {
        "stats",
        "\r\nstats:\r\n Displays a table showing the state of "
        "FreeRTOS\r\n\r\n",
        Command_Stats,
        0,
    },
    {
        "set-model",
        "\r\nset-model <model>:\r\n Set robot model. Expext:I[nfantry], "
        "H[ero], "
        "E[ngineer], D[rone] and S[entry]\r\n\r\n",
        Command_SetModel,
        1,
    },
    {
        "set-pilot",
        "\r\nset-pilot <pilot>:\r\n Set robot pilot. Expext: qs\r\n\r\n",
        Command_SetPilot,
        1,
    },
    {
        "error",
        "\r\nerror:\r\n Get robot error status.\r\n\r\n",
        Command_Error,
        0,
    },
    {
        "motor-id-set",
        "\r\nmotor-id-set:\r\n Enter motor ID quick set mode.\r\n\r\n",
        Command_MotorIDQuitckSet,
        0,
    },
    {
        "reset-robot-id",
        "\r\nreset_robot-id:\r\n Reset Robot ID stored on flash.\r\n\r\n",
        Command_ClearRobotID,
        0,
    },
};

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */
void Task_CLI(void *argument) {
  Task_Param_t *task_param = (Task_Param_t *)argument;

  static char input[MAX_INPUT_LENGTH];
  char *output = FreeRTOS_CLIGetOutputBuffer();
  char rx_char;
  uint16_t index = 0;
  BaseType_t processing = 0;

  /* Register all the commands. */
  int num_commands = sizeof(command_table) / sizeof(CLI_Command_Definition_t);
  for (int j = 0; j < num_commands; j++) {
    FreeRTOS_CLIRegisterCommand(command_table + j);
  }

  /* Command Line Interface. */
  BSP_USB_Printf("Please press ENTER to activate this console.\r\n");
  while (1) {
    BSP_USB_ReadyReceive(osThreadGetId());
    osThreadFlagsWait(SIGNAL_BSP_USB_BUF_RECV, osFlagsWaitAll, osWaitForever);

    rx_char = BSP_USB_ReadChar();

    if (rx_char == '\n' || rx_char == '\r') {
      BSP_USB_Printf("%c", rx_char);
      break;
    }
  }

  BSP_USB_Printf(CLI_WELCOME_MESSAGE);

  BSP_USB_Printf("rm>");
  while (1) {
#ifdef DEBUG
    task_param->stack_water_mark.cli = osThreadGetStackSpace(NULL);
#endif
    /* Wait for input. */
    BSP_USB_ReadyReceive(osThreadGetId());
    osThreadFlagsWait(SIGNAL_BSP_USB_BUF_RECV, osFlagsWaitAll, osWaitForever);

    rx_char = BSP_USB_ReadChar();

    if (rx_char <= 126 && rx_char >= 32) {
      if (index < MAX_INPUT_LENGTH) {
        BSP_USB_Printf("%c", rx_char);
        input[index] = rx_char;
        index++;
      }
    } else {
      if (rx_char == '\n' || rx_char == '\r') {
        BSP_USB_Printf("\r\n");
        if (index > 0) {
          do {
            processing = FreeRTOS_CLIProcessCommand(
                input, output, configCOMMAND_INT_MAX_OUTPUT_SIZE);
            BSP_USB_Printf(output);
            memset(output, 0x00, MAX_INPUT_LENGTH);
            osDelay(50);
          } while (processing != pdFALSE);
          index = 0;
          memset(input, 0x00, MAX_INPUT_LENGTH);
        }
        BSP_USB_Printf("rm>");
      } else if (rx_char == '\b' || rx_char == 0x7Fu) {
        if (index > 0) {
          BSP_USB_Printf("%c", rx_char);
          index--;
          input[index] = 0;
        }
      }
    }
  }
}
