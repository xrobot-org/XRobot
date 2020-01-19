#pragma once

#include "main.h"

#include <string.h>
#include <stdbool.h>

int Board_Delay(uint32_t ms);

float Board_GetTemprater(void);
float Board_GetBatteryVoltage(void);
uint8_t Board_GetHardwareVersion(void);

uint32_t Board_GetCrc32CheckSum(uint32_t *data, uint32_t len);
bool Board_VerifyCrc32CheckSum(uint32_t *data, uint32_t len);
void Board_AppendCrc32CheckSum(uint32_t *data, uint32_t len);

uint32_t Board_get_random_num(void);
int32_t Board_get_random_rangle(int min, int max);

int Board_USBPrint(const char *fmt,...);
