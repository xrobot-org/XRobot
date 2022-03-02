#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "bsp_usb.h"
#include "comp_type.h"
#include "dev.h"

#define term_printf bsp_usb_printf

err_t term_init();

err_t term_update();

bool term_opened();

uint32_t term_avail();

char term_read_char();

err_t term_get_ctrl(uint32_t timeout);

err_t term_give_ctrl();

uint16_t term_read(uint8_t *buffer, uint32_t len);

err_t term_write(uint8_t *buffer, uint32_t len);
