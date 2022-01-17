#pragma once

#include "comp_cmd.h"
#include "comp_utils.h"
#include "dev.h"

typedef struct __packed {
  uint16_t ch_r_x : 11;
  uint16_t ch_r_y : 11;
  uint16_t ch_l_x : 11;
  uint16_t ch_l_y : 11;
  uint8_t sw_r : 2;
  uint8_t sw_l : 2;
  int16_t x;
  int16_t y;
  int16_t z;
  uint8_t press_l;
  uint8_t press_r;
  uint16_t key;
  uint16_t res;
} dr16_data_t;

typedef struct {
  dr16_data_t *data;
} dr16_t;

int8_t dr16_init(dr16_t *dr16);
int8_t dr16_restart(void);

int8_t dr16_start_dma_recv(dr16_t *dr16);
bool dr16_wait_dma_cplt(uint32_t timeout);
int8_t dr16_parse_rc(const dr16_t *dr16, cmd_rc_t *rc);
int8_t dr16_handle_offline(const dr16_t *dr16, cmd_rc_t *rc);
