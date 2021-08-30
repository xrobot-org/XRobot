/*
  错误检测。
*/

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef size_t ErrorDetect_Handle_t;

bool ErrorDetect_Init(ErrorDetect_Handle_t handle, const char *name,
                      uint8_t priority, uint32_t patient_lost,
                      uint32_t patient_work);

void ErrorDetect_Update(ErrorDetect_Handle_t handle, uint32_t sys_time);
void ErrorDetect_Check(uint32_t sys_time);
void ErrorDetect_Detail(char *detail_string, size_t len);
