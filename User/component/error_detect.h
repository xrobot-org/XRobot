/* 
	错误检测。
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    DBUS = 0,
    CHASSIS_M1,
    CHASSIS_M2,
    CHASSIS_M3,
    CHASSIS_M4,
    GIMBAL_YAW,
    GIMBAL_PIT,
    TRIGGER,
    FEED,
    GYRO,
    ACCL,
    MAGN,
    REFEREE,
    ERROR_NUM,
} ErrorDetect_ErrorType_t;

typedef struct {
    uint32_t time_current;
    uint32_t time_last;
    uint32_t period_lost;
    uint32_t period_work;
    uint16_t set_offline_time : 12;
    uint16_t set_online_time : 12;
    uint8_t enable : 1;
    uint8_t priority : 4;
    uint8_t error_exist : 1;
    uint8_t is_lost : 1;
    uint8_t data_is_error : 1;

    float frequency;
    bool (*data_is_error_fun)(void);
    void (*solve_lost_fun)(void);
    void (*solve_data_error_fun)(void);
} ErrorDetect_Error_t;
