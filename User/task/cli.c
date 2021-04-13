/*
  命令行交互界面（Command Line Interface）任务

  实现命令行。

  从USB虚拟串口读取数据，结果也打印到USB虚拟串口。
*/

/* Includes ----------------------------------------------------------------- */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "bsp/can.h"
#include "bsp/usb.h"
#include "component/FreeRTOS_CLI.h"
#include "task.h"
#include "task/user_task.h"

/* Private typedef ---------------------------------------------------------- */
typedef struct {
  uint8_t stage;
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
    "     Q I N G D A O  U N I V E R S I T Y  |  Qu Shen  |  v1.0.0      \r\n"
    " -------------------------------------------------------------------\r\n"
    " FreeRTOS CLI. Type 'help' to view a list of registered commands.   \r\n"
    "\r\n";

static const char *const CLI_START = "qdu-rm>";

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
      /* 每个状态内只允许有一个snprintf相关函数，以保证安全 */
      /* 每个snprintf相关函数必须带有长度限制 */
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
      "-------------------Task list--------------------\r\n"
      "Task          State  Priority  Stack\t#\r\n"
      "------------------------------------------------\r\n";

  static const char *const run_time_header =
      "\r\n"
      "-----------------Run time stats-----------------\r\n"
      "Task            Abs Time        Time\r\n"
      "------------------------------------------------\r\n";

  static const char *const heap_header =
      "\r\n"
      "-------------------Heap stats-------------------\r\n"
      "total(B)\tfree(B)\tused(B)\r\n"
      "------------------------------------------------\r\n";

  static const char *const hardware_header =
      "\r\n"
      "-----------------Hardware stats-----------------\r\n"
      "CPU temp(C)\tBettary(V)\r\n"
      "------------------------------------------------\r\n";

  static const char *const config_header =
      "\r\n"
      "------------------Config stats------------------\r\n"
      "Robot param\tPilot config\r\n"
      "------------------------------------------------\r\n";

  if (out_buffer == NULL) return pdFALSE;
  (void)command_string;
  len -= 1;

  /* 堆区信息的相关内容 */
  HeapStats_t heap_stats;

  static FiniteStateMachine_t fsm;
  switch (fsm.stage) {
    case 0:
      strncpy(out_buffer, task_list_header, len);
      fsm.stage++;
      return pdPASS;
    case 1:
      /* 获取任务的列表 */
      vTaskList(out_buffer);
      fsm.stage++;
      return pdPASS;
    case 2:
      strncpy(out_buffer, run_time_header, len);
      fsm.stage++;
      return pdPASS;
    case 3:
      /* 获得任务的运行时间 */
      vTaskGetRunTimeStats(out_buffer);
      fsm.stage++;
      return pdPASS;
    case 4:
      strncpy(out_buffer, heap_header, len);
      fsm.stage++;
      return pdPASS;
    case 5:
      /* 获得堆区的可用空间 */
      vPortGetHeapStats(&heap_stats);
      snprintf(out_buffer, len, "%d\t\t%d\t%d\r\n", configTOTAL_HEAP_SIZE,
               heap_stats.xAvailableHeapSpaceInBytes,
               configTOTAL_HEAP_SIZE - heap_stats.xAvailableHeapSpaceInBytes);
      fsm.stage++;
      return pdPASS;
    case 6:
      strncpy(out_buffer, hardware_header, len);
      fsm.stage++;
      return pdPASS;
    case 7:
      /* 获取当前CPU温度和电量 */
      snprintf(out_buffer, len, "%f\t%f\r\n", task_runtime.status.cpu_temp,
               task_runtime.status.battery);
      fsm.stage++;
      return pdPASS;
    case 8:
      strncpy(out_buffer, config_header, len);
      fsm.stage++;
      return pdPASS;
    case 9:
      /* 获取机器人和操作手名称 */
      snprintf(out_buffer, len, "%s\t\t%s\r\n",
               task_runtime.cfg.robot_param_name,
               task_runtime.cfg.pilot_cfg_name);
      fsm.stage++;
      return pdPASS;
    default:
      snprintf(out_buffer, len, "\r\n");
      fsm.stage = 0;
      return pdFALSE;
  }
}

static BaseType_t Command_Config(char *out_buffer, size_t len,
                                 const char *command_string) {
  /* 帮助信息，const保证不占用内存空间 */
  static const char *const help_string =
      "\r\n"
      "usage: config <command> [<args>]\r\n"
      "These are commands:\r\n"
      "  help                     Display this message\r\n"
      "  init                     Init config after flashing\r\n"
      "  list <pilot/robot>       List available config\r\n"
      "  set <pilot/robot> <name> Set config\r\n"
      "\r\n";

  /* 用常量表示状态机每个阶段，比数字更清楚 */
  static const uint8_t stage_begin = 0;
  static const uint8_t stage_success = 1;
  static const uint8_t stage_end = 2;

  if (out_buffer == NULL) return pdFALSE;
  len -= 1;

  /* 获取每一段参数的开始地址和长度 */
  BaseType_t command_len, pr_len, name_len;
  const char *command =
      FreeRTOS_CLIGetParameter(command_string, 1, &command_len);
  const char *pr = FreeRTOS_CLIGetParameter(command_string, 2, &pr_len);
  const char *name = FreeRTOS_CLIGetParameter(command_string, 3, &name_len);

  Config_t cfg;

  static FiniteStateMachine_t fsm;
  if (strncmp(command, "help", command_len) == 0) {
    /* config help */
    snprintf(out_buffer, len, "%s", help_string);
    fsm.stage = stage_begin;
    return pdFALSE;

  } else if (strncmp(command, "init", command_len) == 0) {
    if ((pr != NULL) || (name != NULL)) goto command_error;

    /* config init */
    switch (fsm.stage) {
      case stage_begin:
        snprintf(out_buffer, len, "\r\nSet Robot config to default and qs.");
        fsm.stage = stage_success;
        return pdPASS;

      case stage_success:
        /* 初始化获取的操作手配置、机器人参数 */
        memset(&cfg, 0, sizeof(cfg));
        cfg.pilot_cfg = Config_GetPilotCfg("qs");
        cfg.robot_param = Config_GetRobotParam("default");
        snprintf(cfg.robot_param_name, 20, "default");
        snprintf(cfg.pilot_cfg_name, 20, "qs");
        Config_Set(&cfg);
        snprintf(out_buffer, len, "\r\nDone.");
        fsm.stage = stage_end;
        return pdPASS;
    }
  } else if (strncmp(command, "list", command_len) == 0) {
    if ((pr == NULL) || (name != NULL)) goto command_error;

    /* config list */
    static uint8_t i = 0;

    if (strncmp(pr, "pilot", pr_len) == 0) {
      /* config list pilot */
      const Config_PilotCfgMap_t *pilot = Config_GetPilotNameMap();
      switch (fsm.stage) {
        case stage_begin:
          snprintf(out_buffer, len, "\r\nAvailable pilot cfg:");
          fsm.stage = stage_success;
          return pdPASS;
        case stage_success:
          if (pilot[i].name != NULL) {
            snprintf(out_buffer, len, "\r\n  %s", pilot[i].name);
            i++;
            fsm.stage = stage_success;
          } else {
            i = 0;
            fsm.stage = stage_end;
          }
          return pdPASS;
      }
    } else if (strncmp(pr, "robot", pr_len) == 0) {
      /* config list robot */
      const Config_RobotParamMap_t *robot = Config_GetRobotNameMap();
      switch (fsm.stage) {
        case stage_begin:
          snprintf(out_buffer, len, "\r\nAvailable robot params:");
          fsm.stage = stage_success;
          return pdPASS;
        case stage_success:
          if (robot[i].name != NULL) {
            snprintf(out_buffer, len, "\r\n  %s", robot[i].name);
            i++;
            fsm.stage = stage_success;
          } else {
            i = 0;
            fsm.stage = stage_end;
          }
          return pdPASS;
      }
    } else {
      goto command_error;
    }

  } else if (strncmp(command, "set", command_len) == 0) {
    if ((pr == NULL) || (name == NULL)) goto command_error;

    /* config set */
    if (strncmp(pr, "robot", pr_len) == 0) {
      /* config set robot */
      if (fsm.stage == stage_begin) {
        Config_Get(&cfg);
        if (Config_GetRobotParam(name) == NULL) {
          snprintf(out_buffer, len, "\r\nFailed: Unknow model.");
          fsm.stage = stage_end;
          return pdPASS;

        } else {
          snprintf(out_buffer, len, "\r\nSucceed.");
          snprintf(cfg.robot_param_name, 20, "%s", name);
          Config_Set(&cfg);
          fsm.stage = stage_success;
          return pdPASS;
        }
      }
    } else if (strncmp(pr, "pilot", pr_len) == 0) {
      /* config set pilot */
      if (fsm.stage == 0) {
        Config_Get(&cfg);
        if (Config_GetPilotCfg(name) == NULL) {
          snprintf(out_buffer, len, "\r\nFailed: Unknow pilot.");
          fsm.stage = stage_end;
          return pdPASS;
        } else {
          snprintf(out_buffer, len, "\r\nSucceed.");
          snprintf(cfg.pilot_cfg_name, 20, "%s", name);
          Config_Set(&cfg);
          fsm.stage = stage_success;
          return pdPASS;
        }
      }
    } else {
      goto command_error;
    }
    if (fsm.stage == stage_success) {
      snprintf(out_buffer, len,
               "\r\nRestart needed for setting to take effect.");
      fsm.stage = stage_end;
      return pdPASS;
    }
  } /* config set */

  if (fsm.stage == stage_end) {
    snprintf(out_buffer, len, "\r\n\r\n");
    fsm.stage = stage_begin;
    return pdFALSE;
  }

command_error:
  snprintf(out_buffer, len,
           "\r\nconfig: invalid command. See 'config help'.\r\n");
  fsm.stage = stage_begin;
  return pdFALSE;
}

static BaseType_t Command_CaliGyro(char *out_buffer, size_t len,
                                   const char *command_string) {
  if (out_buffer == NULL) return pdFALSE;
  (void)command_string;
  len -= 1;

  /* 陀螺仪校准相关内容 */
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
      snprintf(out_buffer, len, "Please make the MCU stable.\r\n");
      fsm.stage++;
      return pdPASS;
    case 2:
      /* 无陀螺仪数据和校准重试超时时，校准失败 */
      osThreadSuspend(task_runtime.thread.ctrl_gimbal);

      if (osMessageQueueGet(task_runtime.msgq.gimbal.gyro, &gyro, NULL, 5) !=
          osOK) {
        snprintf(out_buffer, len, "Can not get gyro data.\r\n");
        fsm.stage = 7;
        osThreadResume(task_runtime.thread.ctrl_gimbal);

        return pdPASS;
      }
      osThreadResume(task_runtime.thread.ctrl_gimbal);

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
      /* 记录1000组数据，求出平均值作为陀螺仪的3轴零偏 */
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

static BaseType_t Command_SetMechZero(char *out_buffer, size_t len,
                                      const char *command_string) {
  if (out_buffer == NULL) return pdFALSE;
  (void)command_string;
  len -= 1;

  CAN_t can;
  Config_t cfg;

  static FiniteStateMachine_t fsm;
  switch (fsm.stage) {
    case 0:
      snprintf(out_buffer, len, "\r\nStart setting mechanical zero point.\r\n");
      fsm.stage++;
      return pdPASS;
    case 1:
      /* 获取到云台数据，用can上的新的云台机械零点的位置替代旧的位置 */
      Config_Get(&cfg);

      osThreadSuspend(task_runtime.thread.ctrl_gimbal);
      if (osMessageQueueGet(task_runtime.msgq.can.feedback.gimbal, &can, NULL,
                            5) != osOK) {
        snprintf(out_buffer, len, "Can not get gimbal data.\r\n");
        fsm.stage = 2;
        osThreadResume(task_runtime.thread.ctrl_gimbal);
        return pdPASS;
      }
      osThreadResume(task_runtime.thread.ctrl_gimbal);
      cfg.mech_zero.yaw = can.motor.gimbal.named.yaw.rotor_angle;
      cfg.mech_zero.pit = can.motor.gimbal.named.pit.rotor_angle;

      Config_Set(&cfg);
      snprintf(out_buffer, len, "yaw:%f, pitch:%f, rol:%f\r\nDone.",
               cfg.mech_zero.yaw, cfg.mech_zero.pit, cfg.mech_zero.rol);

      fsm.stage = 3;
      return pdPASS;
    case 2:
      snprintf(out_buffer, len, "Calibation failed.\r\n");
      fsm.stage = 3;
      return pdPASS;
    default:
      snprintf(out_buffer, len, "\r\n");
      fsm.stage = 0;
      return pdFALSE;
  }
}

static BaseType_t Command_SetGimbalLim(char *out_buffer, size_t len,
                                       const char *command_string) {
  if (out_buffer == NULL) return pdFALSE;
  (void)command_string;
  len -= 1;

  CAN_t can;
  Config_t cfg;

  static FiniteStateMachine_t fsm;
  switch (fsm.stage) {
    case 0:
      Config_Get(&cfg);
      snprintf(out_buffer, len, "(old)limit:%f\r\n", cfg.gimbal_limit);
      fsm.stage = 1;
      return pdPASS;
    case 1:
      /* 获取云台数据，获取新的限位角并替代旧的限位角 */
      osThreadSuspend(task_runtime.thread.ctrl_gimbal);

      if (osMessageQueueGet(task_runtime.msgq.can.feedback.gimbal, &can, NULL,
                            10) != osOK) {
        osThreadResume(task_runtime.thread.ctrl_gimbal);
        fsm.stage = 3;
        return pdPASS;
      }
      Config_Get(&cfg);
      osThreadResume(task_runtime.thread.ctrl_gimbal);
      cfg.gimbal_limit = can.motor.gimbal.named.pit.rotor_angle;

      Config_Set(&cfg);
      Config_Get(&cfg);
      snprintf(out_buffer, len, "(new)limit:%f\r\nDone.", cfg.gimbal_limit);

      fsm.stage = 2;
      return pdPASS;
    case 2:
      snprintf(out_buffer, len,
               "\r\nRestart needed for setting to take effect.");
      fsm.stage = 5;
      return pdPASS;
    case 3:
      snprintf(out_buffer, len,
               "Calibation failed.\r\nCan not get gimbal data.\r\n");
      fsm.stage = 5;
      return pdPASS;
    case 4:
      snprintf(out_buffer, len, "Unknow options.\r\nPlease check help.");
      fsm.stage = 5;
      return pdPASS;
    default:
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

      snprintf(out_buffer, len, "\r\nXXX.\r\n");


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
        "\r\nstats:\r\n Displays several tables showing the state of "
        "RTOS, system & robot.\r\n\r\n",
        Command_Stats,
        0,
    },
    {
        "config",
        "\r\nconfig:\r\n See 'config help'. \r\n\r\n",
        Command_Config,
        -1,
    },
    {
        "cali-gyro",
        "\r\ncali-gyro:\r\n Calibrates gyroscope to remove zero-offset. Power "
        "off all motors before calibrating!\r\n\r\n",
        Command_CaliGyro,
        0,
    },
    {
        "set-mech-zero",
        "\r\nset-mech-zero:\r\n Sets mechanical zero point for gimbal.\r\n\r\n",
        Command_SetMechZero,
        0,
    },
    {
        "set-gimbal-limit",
        "\r\nset-gimbal-limit:\r\n Move the gimbal to the peak and execute "
        "this command to calibrate the limit of gimbal.\r\n\r\n",
        Command_SetGimbalLim,
        0,
    },
    /*
    {
        "xxx-xxx",
        "\r\nxxx-xxx:\r\n how to use. ",
        Command_XXX,
        1,
    },
    */
};

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * \brief 命令行交互界面
 *
 * \param argument 未使用
 */
void Task_CLI(void *argument) {
  (void)argument;                      /* 未使用argument，消除警告 */
  static char input[MAX_INPUT_LENGTH]; /* 输入字符串缓存 */
  char *output = FreeRTOS_CLIGetOutputBuffer(); /* 输出字符串缓存 */
  char rx_char;                                 /* 接收到的字符 */
  uint16_t index = 0;                           /* 字符串索引值 */
  BaseType_t processing = 0;                    /* 命令行解析控制 */

  /* 注册所有命令 */
  const size_t num_commands =
      sizeof(command_table) / sizeof(CLI_Command_Definition_t);
  for (size_t j = 0; j < num_commands; j++) {
    FreeRTOS_CLIRegisterCommand(command_table + j);
  }

  /* 通过回车键唤醒命令行界面 */
  BSP_USB_Printf("Please press ENTER to activate this console.\r\n");
  while (1) {
    /* 等待接收到新的字符 */
    BSP_USB_ReadyReceive(osThreadGetId());
    osThreadFlagsWait(SIGNAL_BSP_USB_BUF_RECV, osFlagsWaitAll, osWaitForever);

    /* 读取接收到的新字符 */
    rx_char = BSP_USB_ReadChar();

    /* 进行判断 */
    if (rx_char == '\n' || rx_char == '\r') {
      BSP_USB_Printf("%c", rx_char);
      break;
    }
  }

  /* 打印欢迎信息 */
  BSP_USB_Printf(CLI_WELCOME_MESSAGE);

  /* 开始运行命令行界面 */
  BSP_USB_Printf(CLI_START);
  while (1) {
#ifdef DEBUG
    /* 记录任务所使用的的栈空间 */
    task_runtime.stack_water_mark.cli = osThreadGetStackSpace(osThreadGetId());
#endif
    /* 等待输入. */
    BSP_USB_ReadyReceive(osThreadGetId());
    osThreadFlagsWait(SIGNAL_BSP_USB_BUF_RECV, osFlagsWaitAll, osWaitForever);

    /* 读取接收到的新字符 */
    rx_char = BSP_USB_ReadChar();

    if (rx_char <= 126 && rx_char >= 32) {
      /* 如果字符是可显示字符，则直接显式，并存入输入缓存中 */
      if (index < MAX_INPUT_LENGTH) {
        BSP_USB_Printf("%c", rx_char);
        input[index] = rx_char;
        index++;
      }
    } else {
      /* 如果字符是控制字符，则需要进一步判断 */
      if (rx_char == '\n' || rx_char == '\r') {
        /* 如果输入的是回车，则认为命令输入完毕，进行下一步的解析和运行命令 */
        BSP_USB_Printf("\r\n");
        if (index > 0) {
          /* 只在输入缓存有内容时起效 */
          do {
            /* 处理命令 */
            processing = FreeRTOS_CLIProcessCommand(
                input, output, configCOMMAND_INT_MAX_OUTPUT_SIZE);

            BSP_USB_Printf(output);               /* 打印结果 */
            memset(output, 0x00, strlen(output)); /* 清空输出缓存 */
          } while (processing != pdFALSE); /* 是否需要重复运行命令 */
          index = 0; /* 重置索引，准备接收下一段命令 */
          memset(input, 0x00, strlen(input)); /* 清空输入缓存 */
        }
        BSP_USB_Printf(CLI_START);
      } else if (rx_char == '\b' || rx_char == 0x7Fu) {
        /* 如果输入的是退格键则清空一位输入缓存，同时进行界限保护 */
        if (index > 0) {
          BSP_USB_Printf("%c", rx_char);
          index--;
          input[index] = 0;
        }
      }
    }
  }
}
