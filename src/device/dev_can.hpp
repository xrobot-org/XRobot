#pragma once

#include <stdbool.h>

#include "FreeRTOS.h"
#include "bsp_can.h"
#include "comp_ahrs.hpp"
#include "comp_utils.hpp"
#include "dev.hpp"
#include "queue.h"

int8_t can_init(void);
