/*
  AI
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------- */
#include <cmsis_os2.h>
#include <stdbool.h>
#include <stdint.h>

#include "component/ahrs.h"
#include "component/cmd.h"
#include "component/user_math.h"
#include "device/device.h"
#include "device/referee.h"
#include "protocol.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */

typedef struct __packed {
  uint8_t id;
  Protocol_UpPackageReferee_t package;
} AI_UpPackageReferee_t;

typedef struct __packed {
  uint8_t id;
  Protocol_UpPackageMCU_t package;
} AI_UpPackageMCU_t;

typedef struct __packed {
  osThreadId_t thread_alert;

  Protocol_DownPackage_t form_host;

  struct {
    AI_UpPackageReferee_t ref;
    AI_UpPackageMCU_t mcu;
  } to_host;

  CMD_AI_Status_t status;
} AI_t;

/* Exported functions prototypes -------------------------------------------- */
int8_t AI_Init(AI_t *ai);
int8_t AI_Restart(void);

int8_t AI_StartReceiving(AI_t *ai);
bool AI_WaitDmaCplt(void);
int8_t AI_ParseHost(AI_t *ai, CMD_Host_t *cmd_host);
int8_t AI_HandleOffline(AI_t *ai, CMD_Host_t *cmd_host);
int8_t AI_PackMCU(AI_t *ai, const AHRS_Quaternion_t *quat);
int8_t AI_PackRef(AI_t *ai, const Referee_ForAI_t *ref);
int8_t AI_StartSend(AI_t *ai, bool option);
#ifdef __cplusplus
}
#endif
