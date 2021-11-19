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
} AI_UpPackageReferee_t;

typedef struct __packed {
  uint8_t id;
  Protocol_UpPackageMCU_t package;
} AI_UpPackageMCU_t;

typedef struct {
  struct {
    SemaphoreHandle_t recv;
    SemaphoreHandle_t trans;
  } sem;

  bool ref_updated;

  Protocol_DownPackage_t form_host;

  struct {
    AI_UpPackageReferee_t ref;
    AI_UpPackageMCU_t mcu;
  } to_host;

  Game_AI_Mode_t mode;

  bool ai_online;
} AI_t;

typedef struct {
  Game_AI_Mode_t mode;
} AI_UI_t;

int8_t AI_Init(AI_t *ai);
int8_t AI_Restart(void);

bool AI_StartReceiving(AI_t *ai);
bool AI_WaitRecvCplt(AI_t *ai, uint32_t timeout);
bool AI_StartTrans(AI_t *ai);
bool AI_WaitTransCplt(AI_t *ai, uint32_t timeout);
int8_t AI_ParseHost(AI_t *ai);
int8_t AI_HandleOffline(AI_t *ai);
int8_t AI_PackMcuForHost(AI_t *ai, const Quaternion_t *quat);
int8_t AI_PackRefForHost(AI_t *ai, const Referee_ForAI_t *ref);
void AI_PackUI(AI_UI_t *ui, const AI_t *ai);
void AI_PackCMD(AI_t *ai, CMD_Host_t *cmd_host);
