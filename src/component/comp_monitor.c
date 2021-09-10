/**
 * @file comp_monitor.c
 * @author Qu Shen
 * @brief 监视器，用于监视任务运行状态
 * @version 0.1
 * @date 2021-08-31
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "comp_monitor.h"

#include <stddef.h>
#include <string.h>

#define MAX_MONITOR_DETECTOR (10)

Monitor_t detect_list[MAX_MONITOR_DETECTOR];

Monitor_t *Monitor_Create(const char *name, uint8_t priority,
                          uint32_t patient_lost, uint32_t patient_work) {
  for (size_t i = 0; i < MAX_MONITOR_DETECTOR; i++) {
    if (detect_list[i].name == NULL) {
      strncpy(detect_list[i].name, name, MAX_MONITOR_NAME_LEN - 1);
      detect_list[i].name[MAX_MONITOR_NAME_LEN] = '\0';
      detect_list[i].priority = priority;
      detect_list[i].patient_lost = patient_lost;
      detect_list[i].patient_work = patient_work;
      return detect_list + i;
    }
  }
  return NULL;
}

void Monitor_Report(Monitor_t *monitor, uint32_t sys_time) {
  monitor->cycle_time = sys_time - monitor->showup_last;
  monitor->showup_last = sys_time;
  monitor->duration_lost = 0;
  monitor->duration_work += monitor->cycle_time;
}

void Monitor_Examine(uint32_t sys_time) {
  for (size_t i = 0; i < MAX_MONITOR_DETECTOR; i++) {
    if (detect_list[i].name != NULL) {
      if (sys_time - detect_list[i].showup_last > detect_list[i].patient_lost) {
        detect_list[i].lost = true;
        detect_list[i].found_lost = sys_time;
        detect_list[i].duration_work = 0;
      }
    }
  }
}

void Monitor_GetDetailTable(char *detail_string, size_t len) {
  static const char *const header = "\r\n";
  strncpy(detail_string, header, len - 1);
  detail_string[len] = '\0';
}
