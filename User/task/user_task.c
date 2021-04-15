/**
 * @file user_task.c
 * @author Qu Shen (503578404@qq.com)
 * @brief
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 * 保存任务属性：堆栈大小、优先级等
 * 生成任务时使用。
 *
 */

/* Includes ----------------------------------------------------------------- */
#include "task/user_task.h"

/* 机器人运行时的数据 */
Task_Runtime_t task_runtime;

/* 各个任务的参数 */
const osThreadAttr_t attr_init = {
    .name = "init",
    .priority = osPriorityRealtime,
    .stack_size = 256 * 4,
};

const osThreadAttr_t attr_ai = {
    .name = "ai",
    .priority = osPriorityRealtime,
    .stack_size = 128 * 4,
};

const osThreadAttr_t attr_atti_esti = {
    .name = "atti_esti",
    .priority = osPriorityRealtime,
    .stack_size = 256 * 4,
};

const osThreadAttr_t attr_can = {
    .name = "can",
    .priority = osPriorityRealtime,
    .stack_size = 128 * 4,
};

const osThreadAttr_t attr_cap = {
    .name = "cap",
    .priority = osPriorityHigh,
    .stack_size = 128 * 4,
};

const osThreadAttr_t attr_cli = {
    .name = "cli",
    .priority = osPriorityNormal,
    .stack_size = 256 * 4,
};

const osThreadAttr_t attr_command = {
    .name = "command",
    .priority = osPriorityHigh,
    .stack_size = 128 * 4,
};

const osThreadAttr_t attr_ctrl_chassis = {
    .name = "ctrl_chassis",
    .priority = osPriorityAboveNormal,
    .stack_size = 256 * 4,
};

const osThreadAttr_t attr_ctrl_gimbal = {
    .name = "ctrl_gimbal",
    .priority = osPriorityAboveNormal,
    .stack_size = 256 * 4,
};

const osThreadAttr_t attr_ctrl_launcher = {
    .name = "ctrl_launcher",
    .priority = osPriorityAboveNormal,
    .stack_size = 256 * 4,
};

const osThreadAttr_t attr_info = {
    .name = "info",
    .priority = osPriorityLow,
    .stack_size = 128 * 4,
};

const osThreadAttr_t attr_monitor = {
    .name = "monitor",
    .priority = osPriorityBelowNormal,
    .stack_size = 128 * 4,
};

const osThreadAttr_t attr_rc = {
    .name = "rc",
    .priority = osPriorityRealtime,
    .stack_size = 128 * 4,
};

const osThreadAttr_t attr_referee = {
    .name = "referee",
    .priority = osPriorityRealtime,
    .stack_size = 512 * 4,
};
