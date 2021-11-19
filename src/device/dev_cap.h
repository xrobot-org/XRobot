#pragma once

#include <stdbool.h>

#include "FreeRTOS.h"
#include "comp_ui.h"
#include "comp_utils.h"
#include "queue.h"

typedef struct {
  float input_volt;
  float cap_volt;
  float input_curr;
  float target_power;
  float percentage;
} Cap_Feedback_t;

typedef struct {
  float power_limit;
} Cap_Control_t;

typedef struct {
  QueueHandle_t msgq_control;
  QueueHandle_t msgq_feedback;

  Cap_Feedback_t feedback;
} Cap_t;

Err_t Cap_Init(Cap_t *cap);
Err_t Cap_Update(Cap_t *cap, uint32_t timeout);
Err_t Cap_Control(Cap_t *cap, Cap_Control_t *output);
Err_t Cap_HandleOffline(Cap_t *cap);
Err_t Cap_PackUI(const Cap_t *cap, UI_CapUI_t *ui);
