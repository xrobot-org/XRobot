/*
        错误检测。
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  /* Low priority */
  ERROR_DETECT_UNIT_NO_DEV = 0,
  ERROR_DETECT_UNIT_REFEREE,
  ERROR_DETECT_UNIT_CHASSIS_M1,
  ERROR_DETECT_UNIT_CHASSIS_M2,
  ERROR_DETECT_UNIT_CHASSIS_M3,
  ERROR_DETECT_UNIT_CHASSIS_M4,
  ERROR_DETECT_UNIT_TRIGGER,
  ERROR_DETECT_UNIT_FEED,
  ERROR_DETECT_UNIT_GIMBAL_YAW,
  ERROR_DETECT_UNIT_GIMBAL_PIT,
  ERROR_DETECT_UNIT_GYRO,
  ERROR_DETECT_UNIT_ACCL,
  ERROR_DETECT_UNIT_MAGN,
  ERROR_DETECT_UNIT_DBUS,
  ERROR_DETECT_UNIT_NUM,
  /* High priority */
} ErrorDetect_Unit_t;

typedef struct {
  bool enable;
  uint8_t priority;
  uint32_t patient_lost;
  uint32_t patient_work;

  uint32_t showup;
  uint32_t showup_last;
  uint32_t cycle_time;
  uint32_t duration_lost;
  uint32_t duration_work;
  uint32_t found_lost;
  bool error_exist;
  bool is_lost;
  uint8_t data_is_error;

} ErrorDetect_Error_t;

typedef struct {
  ErrorDetect_Error_t error[ERROR_DETECT_UNIT_NUM];
} ErrorDetect_t;

int8_t ErrorDetect_Init(void);
void ErrorDetect_Processing(uint32_t sys_time);
bool ErrorDetect_ErrorExist(ErrorDetect_Unit_t unit);
ErrorDetect_Unit_t ErrorDetect_GetErrorUnit(void);
const ErrorDetect_Error_t *ErrorDetect_GetDetail(ErrorDetect_Unit_t unit);

void ErrorDetect_Update(ErrorDetect_Unit_t unit, uint32_t time_current);

#ifdef __cplusplus
}
#endif
