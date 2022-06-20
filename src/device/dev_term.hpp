#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "bsp_usb.h"
#include "comp_type.hpp"
#include "dev.hpp"

#define term_printf bsp_usb_printf

int8_t term_init();

int8_t term_update();

bool term_opened();

uint32_t term_avail();

char term_read_char();

uint16_t term_read(uint8_t *buffer, uint32_t len);

int8_t term_write(uint8_t *buffer, uint32_t len);
