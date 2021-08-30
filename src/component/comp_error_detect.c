/*
  错误检测。
*/

#include "comp_error_detect.h"

#include <stddef.h>
#include <string.h>

#define MAX_ERROR_DETECTOR (10)
#define MAX_NAME_LEN (20)

typedef struct {
  char name[MAX_NAME_LEN];
  uint8_t priority;
  uint32_t patient_lost;
  uint32_t patient_work;

  bool enable;
  uint32_t showup_last;
  uint32_t cycle_time;
  uint32_t duration_lost;
  uint32_t duration_work;
  uint32_t found_lost;
  bool lost;

} ErrorDetect_t;

ErrorDetect_t list[MAX_ERROR_DETECTOR];

bool ErrorDetect_Init(ErrorDetect_Handle_t handle, const char *name,
                      uint8_t priority, uint32_t patient_lost,
                      uint32_t patient_work) {
  for (size_t i = 0; i < MAX_ERROR_DETECTOR; i++) {
    if (list[i].name == NULL) {
      strncpy(name, list[i].name, MAX_NAME_LEN - 1);
      list[i].name[MAX_NAME_LEN] = '\0';
      list[i].priority = priority;
      list[i].patient_lost = patient_lost;
      list[i].patient_work = patient_work;
      return true;
    }
  }
  return false;
}

void ErrorDetect_Update(ErrorDetect_Handle_t handle, uint32_t sys_time) {
  list[handle].cycle_time = sys_time - list[handle].showup_last;
  list[handle].showup_last = sys_time;
  list[handle].duration_lost = 0;
  list[handle].duration_work += list[handle].cycle_time;
}

void ErrorDetect_Check(uint32_t sys_time) {
  for (size_t i = 0; i < MAX_ERROR_DETECTOR; i++) {
    if (list[i].name != NULL) {
      if (sys_time - list[i].showup_last > list[i].patient_lost) {
        list[i].lost = true;
        list[i].found_lost = sys_time;
        list[i].duration_work = 0;
      }
    }
  }
}

void ErrorDetect_Detail(char *detail_string, size_t len) {
  static const char *const header = "\r\n";
  strncpy(header, detail_string, len - 1);
  detail_string[len] = '\0';
}
