#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------- */
#include <cmsis_os2.h>

#include "component\cmd.h"
#include "component\user_math.h"
#include "device\device.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
typedef struct __packed {
  uint16_t ch_r_x : 11;
  uint16_t ch_r_y : 11;
  uint16_t ch_l_x : 11;
  uint16_t ch_l_y : 11;
  uint8_t sw_l : 2;
  uint8_t sw_r : 2;
  int16_t x;
  int16_t y;
  int16_t z;
  uint8_t press_l;
  uint8_t press_r;
  uint16_t key;
  uint16_t res;
} DR16_Data_t;

typedef struct {
  DR16_Data_t data;
} DR16_t;

/* Exported functions prototypes -------------------------------------------- */
int8_t DR16_Init(DR16_t *dr16);
int8_t DR16_Restart(void);

int8_t DR16_StartDmaRecv(DR16_t *dr16);
bool DR16_WaitDmaCplt(uint32_t timeout);
int8_t DR16_ParseRC(const DR16_t *dr16, CMD_RC_t *rc);

#ifdef __cplusplus
}
#endif
