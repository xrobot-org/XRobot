#pragma once

#include <stdbool.h>

#include "FreeRTOS.h"
#include "bsp_can.h"
#include "comp_ahrs.h"
#include "comp_utils.h"
#include "dev.h"
#include "queue.h"

int8_t can_init();
