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
typedef struct {
  int stage;
} FiniteStateMachine_t;

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
  if (out_buffer == NULL) return pdFALSE;
  (void)command_string; /* 没用到command_string，消除警告 */
  len -= 1;             /* 字符串后面有\0 */

  /* 任务本身相关的内容 */
  uint8_t list[2] = {0x11, 0x22};
  uint16_t force_convert = ((uint16_t *)list)[0];
  uint16_t assembled = (uint16_t)(list[0] | (list[1] << 8));

  static FiniteStateMachine_t fsm; /* 有限状态机 */
  switch (fsm.stage) {
    case 0:
      /* 每个状态内只允许有一个print相关函数，以保证安全 */
      /* 每个print相关函数必须带有长度限制 */
      snprintf(out_buffer, len, "a[2] = {0x11, 0x22}\r\n");
      fsm.stage++;   /* 改变状态机运行状态 */
      return pdPASS; /* 需要继续运行下一状态时返回pdPASS */
    case 1:
      snprintf(out_buffer, len,
               "Force convert to uint16 list, we got: 0x%x\r\n", force_convert);
      fsm.stage++;
      return pdPASS;
    case 2:
      snprintf(out_buffer, len,
               "Manually assemble a[1], a[0], we got: 0x%x\r\n", assembled);
      fsm.stage++;
      return pdPASS;
    case 3:
      if (force_convert == assembled)
        snprintf(out_buffer, len, "Small endian\r\n");
      else
        snprintf(out_buffer, len, "Big endian\r\n");
      fsm.stage++;
      return pdPASS;
    default: /* 结束时状态 */
      snprintf(out_buffer, len, "\r\n");
      fsm.stage = 0;  /* 重置有限状态机 */
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

  static const char *const robot_config_header =
      "\r\n"
      " Robot Model: %s\tRobot Pilot: %s \r\n"
      "\r\n";

  if (out_buffer == NULL) return pdFALSE;
  (void)command_string;
  len -= 1;

  HeapStats_t heap_stats;

  static FiniteStateMachine_t fsm;
  switch (fsm.stage) {
    case 0:
      strncpy(out_buffer, task_list_header, len);
      fsm.stage++;
      return pdPASS;
    case 1:
      vTaskList(out_buffer);
      fsm.stage++;
      return pdPASS;
    case 2:
      strncpy(out_buffer, run_time_header, len);
      fsm.stage++;
      return pdPASS;
    case 3:
      vTaskGetRunTimeStats(out_buffer);
      fsm.stage++;
      return pdPASS;
    case 4:
      strncpy(out_buffer, heap_header, len);
      fsm.stage++;
      return pdPASS;
    case 5:
      vPortGetHeapStats(&heap_stats);
      snprintf(out_buffer, len, "%d\t\t%d\t%d\r\n", configTOTAL_HEAP_SIZE,
               heap_stats.xAvailableHeapSpaceInBytes,
               configTOTAL_HEAP_SIZE - heap_stats.xAvailableHeapSpaceInBytes);
      fsm.stage++;
      return pdPASS;
    case 6:
      snprintf(out_buffer, len, "\r\nBettary: %.2f %%\r\n",
               task_runtime.status.battery);
      fsm.stage++;
      return pdPASS;
    case 7:
      snprintf(out_buffer, len, "\r\nCPU temp: %0.2f C\r\n",
               task_runtime.status.cpu_temp);
      fsm.stage++;
      return pdPASS;
    case 8:
      snprintf(out_buffer, len, robot_config_header,
               Config_GetNameByModel(task_runtime.robot_id.model),
               Config_GetNameByPilot(task_runtime.robot_id.pilot));
      fsm.stage++;
      return pdPASS;
    default:
      snprintf(out_buffer, len, "\r\n");
      fsm.stage = 0;
      return pdFALSE;
  }
}

static BaseType_t Command_SetModel(char *out_buffer, size_t len,
                                   const char *command_string) {
  const char *param;
  BaseType_t param_len;
  Config_t cfg;

  if (out_buffer == NULL) return pdFALSE;
  len -= 1;

  param = FreeRTOS_CLIGetParameter(command_string, 1, &param_len);
  if (param == NULL) return pdFALSE;

  static FiniteStateMachine_t fsm;
  switch (fsm.stage) {
    case 0:
      snprintf(out_buffer, len, "Set robot model to: ");
      fsm.stage = 1;
      return pdPASS;
    case 1:
      Config_Get(&cfg);
      if ((cfg.model = Config_GetModelByName(param)) == ROBOT_MODEL_NUM) {
        fsm.stage = 2;
        return pdPASS;
      } else {
        snprintf(out_buffer, len, "%s", Config_GetNameByModel(cfg.model));
        Config_Set(&cfg);
        fsm.stage = 3;
        return pdPASS;
      }
    case 2:
      snprintf(out_buffer, len,
               "Unknow model.\r\nCheck help for avaliable options.");
      fsm.stage = 4;
      return pdPASS;
    case 3:
      snprintf(out_buffer, len,
               "\r\nRestart needed for setting to take effect.");
      fsm.stage = 4;
      return pdPASS;
    default:
      snprintf(out_buffer, len, "\r\n");
      fsm.stage = 0;
      return pdFALSE;
  }
}

static BaseType_t Command_SetPilot(char *out_buffer, size_t len,
                                   const char *command_string) {
  const char *param;
  BaseType_t param_len;
  Config_t cfg;

  if (out_buffer == NULL) return pdFALSE;
  len -= 1;

  param = FreeRTOS_CLIGetParameter(command_string, 1, &param_len);
  if (param == NULL) return pdFALSE;

  static FiniteStateMachine_t fsm;
  switch (fsm.stage) {
    case 0:
      snprintf(out_buffer, len, "Set robot pilot to: ");
      fsm.stage = 1;
      return pdPASS;
    case 1:
      Config_Get(&cfg);
      if ((cfg.pilot = Config_GetPilotByName(param)) == ROBOT_PILOT_NUM) {
        fsm.stage = 2;
        return pdPASS;
      } else {
        snprintf(out_buffer, len, "%s", Config_GetNameByPilot(cfg.pilot));
        Config_Set(&cfg);
        fsm.stage = 3;
        return pdPASS;
      }
    case 2:
      snprintf(out_buffer, len,
               "Unauthorized pilot.\r\nCheck help for avaliable options.");
      fsm.stage = 4;
      return pdPASS;
    case 3:
      snprintf(out_buffer, len,
               "\r\nRestart needed for setting to take effect.");
      fsm.stage = 4;
      return pdPASS;
    default:
      snprintf(out_buffer, len, "\r\n");
      fsm.stage = 0;
      return pdFALSE;
  }
}

static BaseType_t Command_Error(char *out_buffer, size_t len,
                                const char *command_string) {
  if (out_buffer == NULL) return pdFALSE;
  (void)command_string;
  len -= 1;

  static FiniteStateMachine_t fsm;
  switch (fsm.stage) {
    case 0:
      snprintf(out_buffer, len, "\r\nError status.");
      fsm.stage++;
      return pdPASS;
    case 1:
      // TODO
      fsm.stage++;
      return pdPASS;
    default:
      snprintf(out_buffer, len, "\r\n");
      fsm.stage = 0;
      return pdFALSE;
  }
}

static BaseType_t Command_MotorIDQuitckSet(char *out_buffer, size_t len,
                                           const char *command_string) {
  if (out_buffer == NULL) return pdFALSE;
  (void)command_string;
  len -= 1;

  static FiniteStateMachine_t fsm;
  switch (fsm.stage) {
    case 0:
      snprintf(out_buffer, len, "\r\nEnter ID quick set mode.");
      fsm.stage++;
      return pdPASS;
    case 1:
      CAN_Motor_QuickIdSetMode();
      snprintf(out_buffer, len, "\r\nDone.");
      fsm.stage++;
      return pdPASS;
    default:
      snprintf(out_buffer, len, "\r\n");
      fsm.stage = 0;
      return pdFALSE;
  }
}

static BaseType_t Command_ClearConfig(char *out_buffer, size_t len,
                                      const char *command_string) {
  if (out_buffer == NULL) return pdFALSE;
  (void)command_string;
  len -= 1;

  Config_t cfg;

  static FiniteStateMachine_t fsm;
  switch (fsm.stage) {
    case 0:
      snprintf(out_buffer, len, "\r\nReset Robot ID stored on flash.");
      fsm.stage++;
      return pdPASS;
    case 1:
      memset(&cfg, 0, sizeof(Config_t));
      Config_Set(&cfg);
      snprintf(out_buffer, len, "\r\nDone.");
      fsm.stage++;
      return pdPASS;
    default:
      snprintf(out_buffer, len, "\r\n");
      fsm.stage = 0;
      return pdFALSE;
  }
}

static BaseType_t Command_CaliGyro(char *out_buffer, size_t len,
                                   const char *command_string) {
  if (out_buffer == NULL) return pdFALSE;
  (void)command_string;
  len -= 1;

  Config_t cfg;
  AHRS_Gyro_t gyro;
  uint16_t count = 0;
  static float x = 0.0f;
  static float y = 0.0f;
  static float z = 0.0f;
  static uint8_t retry = 0;

  static FiniteStateMachine_t fsm;
  switch (fsm.stage) {
    case 0:
      snprintf(out_buffer, len, "\r\nStart gyroscope calibration.\r\n");
      fsm.stage++;
      return pdPASS;
    case 1:
      snprintf(out_buffer, len, "Please make the controller stable.\r\n");
      fsm.stage++;
      return pdPASS;
    case 2:
      if (osMessageQueueGet(task_runtime.msgq.gimbal.gyro, &gyro, NULL, 5) !=
          osOK) {
        snprintf(out_buffer, len, "Can not get gyro data.\r\n");
        fsm.stage = 7;
        return pdPASS;
      }
      if (BMI088_GyroStable(&gyro)) {
        snprintf(out_buffer, len,
                 "Controller is stable. Start calibation.\r\n");
        fsm.stage++;
        return pdPASS;
      } else {
        retry++;
        if (retry < 3) {
          snprintf(out_buffer, len, "Controller is unstable.\r\n");
          fsm.stage = 1;
        } else {
          fsm.stage = 7;
        }
        return pdPASS;
      }
    case 3:
      snprintf(out_buffer, len, "Calibation in progress.\r\n");
      Config_Get(&cfg);
      cfg.cali.bmi088.gyro_offset.x = 0.0f;
      cfg.cali.bmi088.gyro_offset.y = 0.0f;
      cfg.cali.bmi088.gyro_offset.z = 0.0f;
      Config_Set(&cfg);
      while (count < 1000) {
        bool data_new = (osMessageQueueGet(task_runtime.msgq.gimbal.gyro, &gyro,
                                           NULL, 5) == osOK);
        bool data_good = BMI088_GyroStable(&gyro);
        if (data_new && data_good) {
          x += gyro.x;
          y += gyro.y;
          z += gyro.z;
          count++;
        }
      }
      x /= (float)count;
      y /= (float)count;
      z /= (float)count;
      fsm.stage++;
      return pdPASS;
    case 4:
      snprintf(out_buffer, len,
               "Calibation finished. Write result to flash.\r\n");
      Config_Get(&cfg);
      cfg.cali.bmi088.gyro_offset.x = x;
      cfg.cali.bmi088.gyro_offset.y = y;
      cfg.cali.bmi088.gyro_offset.z = z;
      Config_Set(&cfg);
      fsm.stage++;
      return pdPASS;
    case 5:
      snprintf(out_buffer, len, "x:%.5f; y:%.5f; z:%.5f;\r\n", x, y, z);
      fsm.stage++;
      return pdPASS;
    case 6:
      snprintf(out_buffer, len, "Calibation done.\r\n");
      fsm.stage = 8;
      return pdPASS;
    case 7:
      snprintf(out_buffer, len, "Calibation failed.\r\n");
      fsm.stage = 8;
      return pdPASS;
    default:
      x = 0.0f;
      y = 0.0f;
      z = 0.0f;
      retry = 0;
      snprintf(out_buffer, len, "\r\n");
      fsm.stage = 0;
      return pdFALSE;
  }
}

/*
static BaseType_t Command_XXX(char *out_buffer, size_t len,
                                           const char *command_string) {
  if (out_buffer == NULL) return pdFALSE;
  (void)command_string;
  len -= 1;

  static FiniteStateMachine_t fsm;
  switch (fsm.stage) {
    case 0:

      snprintf(out_buffer, len, "\r\nXXX.");


      fsm.stage++;
      return pdPASS;
    case 1:
      XXX();
      snprintf(out_buffer, len, "\r\nDone.");


      fsm.stage++;
      return pdPASS;

    default:
      snprintf(out_buffer, len, "\r\n");
      fsm.stage = 0;
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
        "H[ero], E[ngineer], D[rone] and S[entry]\r\n\r\n",
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
        "reset-config",
        "\r\nreset-config:\r\n Reset Robot config stored on flash.\r\n\r\n",
        Command_ClearConfig,
        0,
    },
    {
        "cali-gyro",
        "\r\ncali-gyro:\r\n Calibrate gyroscope. Remove zero offset.\r\n\r\n",
        Command_CaliGyro,
        0,
    },
};

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */
void Task_CLI(void *argument) {
  (void)argument;
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
    task_runtime.stack_water_mark.cli = osThreadGetStackSpace(NULL);
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
            memset(output, 0x00, strlen(output));
          } while (processing != pdFALSE);
          index = 0;
          memset(input, 0x00, strlen(input));
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
