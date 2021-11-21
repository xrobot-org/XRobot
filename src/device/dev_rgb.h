#pragma once

#include <stdint.h>

#include "comp_type.h"
#include "dev_led.h"

err_t rgb_set_color(color_hex_t color, led_status_t s);
