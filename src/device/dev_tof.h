#pragma once

#include <stdbool.h>

#include "FreeRTOS.h"
#include "comp_type.h"
#include "queue.h"

typedef struct {
  float dist;
  uint8_t status;
  uint16_t signal_strength;
} TOF_Feedback_t;

typedef struct {
  QueueHandle_t msgq_feedback;
  TOF_Feedback_t feedback;
} TOF_t;

Err_t TOF_Init(TOF_t *tof);
Err_t TOF_Update(TOF_t *tof, uint32_t timeout);
Err_t TOF_HandleOffline(TOF_t *tof);
