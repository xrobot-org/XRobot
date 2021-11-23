#pragma once

#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"

#define CAN_DATA_SIZE (8)

typedef enum {
  CAN_PERIPH_1,
  CAN_PERIPH_2,
} can_periph_t;

typedef struct {
  uint32_t can_id;
  uint8_t data[CAN_DATA_SIZE];
} can_tx_item_t;

typedef struct {
  uint32_t index;
  uint8_t data[CAN_DATA_SIZE];
} can_rx_item_t;

typedef struct {
  can_periph_t periph;
  uint32_t can_id;
  uint32_t len;
} can_rx_group_t;

QueueHandle_t can_register_rx_group(const can_rx_group_t *group);
