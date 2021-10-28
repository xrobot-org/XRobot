#pragma once

#include <stddef.h>
#include <stdint.h>

void *BSP_Malloc(size_t size);
void BSP_Free(void *pv);
