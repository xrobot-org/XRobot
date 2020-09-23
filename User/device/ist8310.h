#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------- */
#include <cmsis_os2.h>
#include <stdbool.h>
#include <stdint.h>

#include "component\ahrs.h"
#include "device\device.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
typedef struct {
  AHRS_Magn_t magn;

  struct {
    float x;
    float y;
    float z;
  } magn_offset;

  struct {
    float x;
    float y;
    float z;
  } magn_scale;
} IST8310_t;

/* Exported functions prototypes -------------------------------------------- */
int8_t IST8310_Init(IST8310_t *ist8310);
int8_t IST8310_Restart(void);

bool IST8310_WaitNew(uint32_t timeout);
int8_t IST8310_StartDmaRecv();
uint32_t IST8310_WaitDmaCplt();
int8_t IST8310_Parse(IST8310_t *ist8310);

#ifdef __cplusplus
}
#endif
