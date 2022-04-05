#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "comp_ahrs.h"
#include "comp_cmd.h"
#include "comp_utils.h"
#include "dev.h"
#include "dev_referee.h"
#include "protocol.h"
#include "semphr.h"

typedef struct __packed {
  uint8_t id;
  Protocol_UpPackageReferee_t package;
} ai_up_pckage_referee_t;

typedef struct __packed {
  uint8_t id;
  Protocol_UpPackageMCU_t package;
} ai_up_pckage_mcu_t;

typedef struct {
  bool ref_updated;

  Protocol_DownPackage_t form_host;

  struct {
    ai_up_pckage_referee_t ref;
    ai_up_pckage_mcu_t mcu;
  } to_host;

  struct {
    SemaphoreHandle_t data_ready;
  } sem;

  bool online;
  uint32_t last_online_time;
} ai_t;

int8_t ai_init(ai_t *ai);
int8_t ai_restart(void);

bool ai_start_receiving(ai_t *ai);
int8_t ai_read_host(ai_t *ai);
bool ai_wait_recv_cplt(ai_t *ai);
bool ai_start_trans(ai_t *ai);
int8_t ai_parse_host(ai_t *ai, uint32_t tick);
int8_t ai_handle_offline(ai_t *ai, uint32_t tick);
int8_t ai_pack_mcu_for_host(ai_t *ai, const quaternion_t *quat);
int8_t ai_pack_ref_for_host(ai_t *ai, const referee_for_ai_t *ref);
void ai_pack_cmd(ai_t *ai, cmd_host_t *cmd_host, eulr_t *eulr);
