/**
 * @file comp_monitor.h
 * @author Qu Shen
 * @brief 监视器，用于监视任务运行状态
 * @version 0.1
 * @date 2021-08-31
 *
 * @copyright Copyright (c) 2021
 *
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef size_t Monitor_Handle_t;

bool Monitor_Init(Monitor_Handle_t handle, const char *name, uint8_t priority,
                  uint32_t patient_lost, uint32_t patient_work);

void Monitor_Report(Monitor_Handle_t handle, uint32_t sys_time);
void Monitor_Examine(uint32_t sys_time);
void Monitor_GetDetailTable(char *detail_string, size_t len);
