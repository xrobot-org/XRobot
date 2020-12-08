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

#include "component\cmd.h"
#include "component\user_math.h"
#include "device\device.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
typedef uint16_t Ai_Tail_t;

typedef enum {
  AI_CMD_ID_COMMAND = 0x0001,
} AI_CMDID_t;

typedef struct __packed {
  uint8_t sof;
  uint16_t data_length;
  uint8_t seq;
  uint8_t crc8;
} AI_Header_t;

typedef struct {
  osThreadId_t thread_alert;

  AI_CMDID_t cmd_id;

  CMD_AI_t command;

} AI_t;

/* Exported functions prototypes -------------------------------------------- */
int8_t AI_Init(AI_t *ai, osThreadId_t thread_alert);
bool AI_WaitDmaCplt(void);
int8_t AI_Restart(void);

int8_t AI_StartReceiving(AI_t *ai);
int8_t AI_Parse(AI_t *ai);

#ifdef __cplusplus
}
#endif
