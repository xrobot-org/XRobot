/**
 * @file cli.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 运行命令行交互界面(Command Line Interface)
 * @version 1.0.0
 * @date 2021-04-14
 *
 * @copyright Copyright (c) 2021
 *
 * 在USB虚拟串口上实现CLI
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "dev_term.h"
#include "om.h"
#include "task.h"
#include "thd.h"

typedef struct {
  uint8_t stage;
} finite_state_machine_t;

#define MAX_INPUT_LENGTH 64

static runtime_t *runtime;

static om_suber_t *gyro_sub;
static om_suber_t *gimbal_motor_yaw_sub;
static om_suber_t *gimbal_motor_pit_sub;

static vector3_t gyro;
static motor_feedback_group_t motor_fb;

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
static BaseType_t command_endian(char *out_buffer, size_t len,
                                 const char *command_string) {
  if (out_buffer == NULL) return pdFALSE;
  RM_UNUSED(command_string); /* 没用到command_string，消除警告 */
  len -= 1;                  /* 字符串后面有\0 */

  /* 线程本身相关的内容 */
  uint8_t list[2] = {0x11, 0x22};
  uint16_t force_convert = ((uint16_t *)list)[0];
  uint16_t assembled = (uint16_t)(list[0] | (list[1] << 8));

  static finite_state_machine_t fsm; /* 有限状态机 */
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

static BaseType_t command_stats(char *out_buffer, size_t len,
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

  static const char *const firmware_header =
      "\r\n"
      "------------------Firmware stats------------------\r\n";

  if (out_buffer == NULL) return pdFALSE;
  RM_UNUSED(command_string);
  len -= 1;

  /* 堆区信息的相关内容 */
  HeapStats_t heap_stats;

  static finite_state_machine_t fsm;
  switch (fsm.stage) {
    case 0:
      strncpy(out_buffer, task_list_header, len);
      fsm.stage++;
      return pdPASS;
    case 1:
      /* 获取线程的列表 */
      vTaskList(out_buffer);
      fsm.stage++;
      return pdPASS;
    case 2:
      strncpy(out_buffer, run_time_header, len);
      fsm.stage++;
      return pdPASS;
    case 3:
      /* 获得线程的运行时间 */
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
      snprintf(out_buffer, len, "%f\t%f\r\n", runtime->status.cpu_temp,
               runtime->status.battery);
      fsm.stage++;
      return pdPASS;
    case 8:
      strncpy(out_buffer, config_header, len);
      fsm.stage++;
      return pdPASS;
    case 9:
      /* 获取机器人和操作手名称 */
      snprintf(out_buffer, len, "%s\t\t%s\r\n", runtime->cfg.robot_param->name,
               runtime->cfg.pilot_cfg->name);
      fsm.stage++;
      return pdPASS;
    case 10:
      strncpy(out_buffer, firmware_header, len);
      fsm.stage++;
      return pdPASS;
    case 11:
      /* 固件相关 */
      snprintf(out_buffer, len, "Build time: %s %s\r\n", __DATE__, __TIME__);
      fsm.stage++;
      return pdPASS;
    default:
      snprintf(out_buffer, len, "\r\n");
      fsm.stage = 0;
      return pdFALSE;
  }
}

static BaseType_t command_cali_gyro(char *out_buffer, size_t len,
                                    const char *command_string) {
  if (out_buffer == NULL) return pdFALSE;
  RM_UNUSED(command_string);
  len -= 1;

  /* 陀螺仪校准相关内容 */
  config_t cfg;
  uint16_t count = 0;
  static float x = 0.0f;
  static float y = 0.0f;
  static float z = 0.0f;
  static uint8_t retry = 0;

  static finite_state_machine_t fsm;
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
      if (gyro_is_stable(&gyro)) {
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
      while (count < 2000) {
        bool data_new = om_suber_dump(gyro_sub, false) == OM_OK;
        bool data_good = gyro_is_stable(&gyro);
        if (data_new && data_good) {
          x += gyro.x;
          y += gyro.y;
          z += gyro.z;
          count++;
        }
        vTaskDelay(1);
      }
      x /= (float)count;
      y /= (float)count;
      z /= (float)count;
      fsm.stage++;
      return pdPASS;
    case 4:
      snprintf(out_buffer, len,
               "Calibation finished. Write result to flash.\r\n");
      config_get(&cfg);
      cfg.cali.bmi088.gyro_offset.x += x;
      cfg.cali.bmi088.gyro_offset.y += y;
      cfg.cali.bmi088.gyro_offset.z += z;
      config_set(&cfg);
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

static BaseType_t command_set_mech_zero(char *out_buffer, size_t len,
                                        const char *command_string) {
  if (out_buffer == NULL) return pdFALSE;
  RM_UNUSED(command_string);
  len -= 1;

  config_t cfg;

  static finite_state_machine_t fsm;
  switch (fsm.stage) {
    case 0:
      snprintf(out_buffer, len, "\r\nStart setting mechanical zero point.\r\n");
      fsm.stage++;
      return pdPASS;
    case 1:
      /* 获取到云台数据，用can上的新的云台机械零点的位置替代旧的位置 */
      config_get(&cfg);

      if (om_suber_dump(gimbal_motor_yaw_sub, false) != OM_OK) {
        snprintf(out_buffer, len, "Can not get gimbal data.\r\n");
        fsm.stage = 2;
        return pdPASS;
      }
      cfg.gimbal_mech_zero.yaw = motor_fb.as_gimbal_yaw.yaw.rotor_abs_angle;
      if (om_suber_dump(gimbal_motor_pit_sub, false) != OM_OK) {
        snprintf(out_buffer, len, "Can not get gimbal data.\r\n");
        fsm.stage = 2;
        return pdPASS;
      }
      cfg.gimbal_mech_zero.pit = motor_fb.as_gimbal_pit.pit.rotor_abs_angle;

      config_set(&cfg);
      snprintf(out_buffer, len, "yaw:%f, pitch:%f, rol:%f\r\nDone.",
               cfg.gimbal_mech_zero.yaw, cfg.gimbal_mech_zero.pit,
               cfg.gimbal_mech_zero.rol);

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

static BaseType_t command_set_gimbal_lim(char *out_buffer, size_t len,
                                         const char *command_string) {
  if (out_buffer == NULL) return pdFALSE;
  RM_UNUSED(command_string);
  len -= 1;

  config_t cfg;

  static finite_state_machine_t fsm;
  switch (fsm.stage) {
    case 0:
      config_get(&cfg);
      snprintf(out_buffer, len, "(old)limit:%f\r\n", cfg.gimbal_limit);
      fsm.stage = 1;
      return pdPASS;
    case 1:
      /* 获取云台数据，获取新的限位角并替代旧的限位角 */
      if (om_suber_dump(gimbal_motor_pit_sub, false) != OM_OK) {
        fsm.stage = 3;
        return pdPASS;
      }
      config_get(&cfg);
      cfg.gimbal_limit = motor_fb.as_gimbal_pit.pit.rotor_abs_angle;

      config_set(&cfg);
      config_get(&cfg);
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

om_status_t _command_list_topic(om_topic_t *topic, void *arg) {
  om_topic_t **index = (om_topic_t **)arg;

  static bool next = false;

  if (*index == NULL) {
    next = false;
    *index = topic;
    return OM_ERROR;
  }

  if (*index == topic && !next) {
    next = true;
    return OM_OK;
  }

  if (next) {
    next = false;
    *index = topic;
    return OM_ERROR;
  }

  return OM_OK;
}

static BaseType_t command_list_topic(char *out_buffer, size_t len,
                                     const char *command_string) {
  if (out_buffer == NULL) return pdFALSE;
  RM_UNUSED(command_string);
  len -= 1;

  static finite_state_machine_t fsm;
  static om_topic_t *index = NULL;
  static bool end = false;
  switch (fsm.stage) {
    case 0:
      end = false;
      index = NULL;
      snprintf(out_buffer, len, "\r\nNow:%fs.\r\n",
               (xTaskGetTickCount() / pdMS_TO_TICKS(1)) / 1000.0f);
      om_msg_for_each(_command_list_topic, &index);

      fsm.stage++;
      return pdPASS;
    case 1:
      if (om_msg_for_each(_command_list_topic, &index) == OM_OK) end = true;

      om_print_topic_message(index, out_buffer, len);
      fsm.stage++;

      return pdPASS;
    case 2: {
      uint32_t time = om_msg_get_last_time(index);
      snprintf(out_buffer, len, "\tLast msg time:%f\r\n",
               (time / pdMS_TO_TICKS(1)) / 1000.0f);
      if (!end)
        fsm.stage = 1;
      else
        fsm.stage++;
      return pdPASS;
      break;
    }
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
  RM_UNUSED(command_string);
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
        command_endian,
        0,
    },
    {
        "stats",
        "\r\nstats:\r\n Displays several tables showing the state of "
        "RTOS, system & robot.\r\n\r\n",
        command_stats,
        0,
    },
    {
        "cali-gyro",
        "\r\ncali-gyro:\r\n Calibrates gyroscope to remove zero-offset. Power "
        "off all motors before calibrating!\r\n\r\n",
        command_cali_gyro,
        0,
    },
    {
        "set-mech-zero",
        "\r\nset-mech-zero:\r\n Sets mechanical zero point for gimbal.\r\n\r\n",
        command_set_mech_zero,
        0,
    },
    {
        "set-gimbal-limit",
        "\r\nset-gimbal-limit:\r\n Move the gimbal to the peak and execute "
        "this command to calibrate the limit of gimbal.\r\n\r\n",
        command_set_gimbal_lim,
        0,
    },
    {
        "list-topic",
        "\r\nlist-topic:\r\n Show all topics info in the topic list\r\n\r\n",
        command_list_topic,
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

void thd_cli(void *arg) {
  runtime = arg;
  static char input[MAX_INPUT_LENGTH];          /* 输入字符串缓存 */
  char *output = FreeRTOS_CLIGetOutputBuffer(); /* 输出字符串缓存 */
  char rx_char;                                 /* 接收到的字符 */
  uint16_t index = 0;                           /* 字符串索引值 */
  BaseType_t processing = 0;                    /* 命令行解析控制 */

  /* 注册所有命令 */
  for (size_t j = 0; j < ARRAY_LEN(command_table); j++) {
    FreeRTOS_CLIRegisterCommand(command_table + j);
  }

  while (1) {
    /* 获取终端控制权 */
    if (term_get_ctrl(0xff) != RM_OK) {
      continue;
    }

    /* 等待连接 */
    if (!term_opened()) {
      term_give_ctrl();
      vTaskDelay(10);
      continue;
    }

    /* 连接成功 */
    break;
  }

  /* 初始化所需数据 */
  om_topic_t *gyro_tp = om_find_topic("gimbal_gyro", UINT32_MAX);
  om_topic_t *yaw_tp = om_find_topic("gimbal_yaw_motor_fb", UINT32_MAX);
  om_topic_t *pit_tp = om_find_topic("gimbal_pit_motor_fb", UINT32_MAX);

  gyro_sub = om_subscript(gyro_tp, OM_PRASE_VAR(gyro));
  gimbal_motor_yaw_sub = om_subscript(yaw_tp, OM_PRASE_VAR(motor_fb));
  gimbal_motor_pit_sub = om_subscript(pit_tp, OM_PRASE_VAR(motor_fb));

  /* 通过回车键唤醒命令行界面 */
  term_printf("Please press ENTER to activate this console.\r\n");

  while (1) {
    /* 等待新数据 */
    if (!term_avail()) {
      vTaskDelay(100);
      continue;
    }

    /* 读取接收到的新字符 */
    rx_char = term_read_char();

    /* 进行判断 */
    if (rx_char == '\n' || rx_char == '\r') {
      term_printf("%c", rx_char);
      break;
    } else {
      term_give_ctrl();
    }
  }

  /* 打印欢迎信息 */
  term_printf(CLI_WELCOME_MESSAGE);

  /* 开始运行命令行界面 */
  term_printf(CLI_START);
  while (1) {
    if (!term_avail()) {
      vTaskDelay(1);
      continue;
    }
    /* 读取接收到的新字符 */
    rx_char = term_read_char();

    if (rx_char <= 126 && rx_char >= 32) {
      /* 如果字符是可显示字符，则直接显式，并存入输入缓存中 */
      if (index < MAX_INPUT_LENGTH) {
        term_printf("%c", rx_char);
        input[index] = rx_char;
        index++;
      }
    } else {
      /* 如果字符是控制字符，则需要进一步判断 */
      if (rx_char == '\n' || rx_char == '\r') {
        /* 如果输入的是回车，则认为命令输入完毕，进行下一步的解析和运行命令 */
        term_printf("\r\n");
        if (index > 0) {
          /* 只在输入缓存有内容时起效 */
          do {
            /* 处理命令 */
            processing = FreeRTOS_CLIProcessCommand(
                input, output, configCOMMAND_INT_MAX_OUTPUT_SIZE);

            term_printf("%s", output);            /* 打印结果 */
            memset(output, 0x00, strlen(output)); /* 清空输出缓存 */
          } while (processing != pdFALSE); /* 是否需要重复运行命令 */
          index = 0; /* 重置索引，准备接收下一段命令 */
          memset(input, 0x00, strlen(input)); /* 清空输入缓存 */
        }
        term_printf(CLI_START);
      } else if (rx_char == '\b' || rx_char == 0x7Fu) {
        /* 如果输入的是退格键则清空一位输入缓存，同时进行界限保护 */
        if (index > 0) {
          term_printf("%c", rx_char);
          index--;
          input[index] = 0;
        }
      }
    }
  }
}
THREAD_DECLEAR(thd_cli, 384, 1);
