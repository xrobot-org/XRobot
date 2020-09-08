/*
        错误检测。
*/

#include "error_detect.h"

#include <stddef.h>
#include <string.h>

#include "bsp/mm.h"

static ErrorDetect_t ged;
static bool inited = false;

int8_t ErrorDetect_Init(void) {
  if (inited) return -1;

  memset(&ged, 0x00, sizeof(ged));

  for (uint8_t i = 0; i < ERROR_DETECT_UNIT_NUM; i++) {
    ged.error[i].enable = true;
    ged.error[i].priority = i;
    ged.error[i].patient_lost = 500;
    ged.error[i].patient_work = 500;
  }
  return 0;
}

void ErrorDetect_Processing(uint32_t sys_time) {
  for (uint8_t i = 0; i < ERROR_DETECT_UNIT_NUM; i++) {
    if (!ged.error[i].enable) continue;

    if (sys_time - ged.error[i].showup > ged.error[i].patient_lost) {
      ged.error[i].is_lost = true;
      ged.error[i].found_lost = sys_time;
    } else if (sys_time - ged.error[i].showup > ged.error[i].patient_lost) {
    } else {
      ged.error[i].cycle_time = ged.error[i].showup - ged.error[i].showup_last;
    }
  }
}

bool ErrorDetect_ErrorExist(ErrorDetect_Unit_t unit) {
  if (unit == ERROR_DETECT_UNIT_NO_DEV) {
    for (uint8_t i = ERROR_DETECT_UNIT_NUM; i > 0; i--) {
      if (ged.error[i].error_exist) return true;
    }
    return false;
  } else {
    return ged.error[unit].error_exist;
  }
}

ErrorDetect_Unit_t ErrorDetect_GetErrorUnit(void) {
  for (uint8_t i = ERROR_DETECT_UNIT_NUM; i > 0; i--) {
    if (ged.error[i].error_exist) return i;
  }
  return ERROR_DETECT_UNIT_NO_DEV;
}

const ErrorDetect_Error_t *ErrorDetect_GetDetail(ErrorDetect_Unit_t unit) {
  return &ged.error[unit];
}

void ErrorDetect_Update(ErrorDetect_Unit_t unit, uint32_t time_current) {
  ged.error[unit].showup = time_current;
}
