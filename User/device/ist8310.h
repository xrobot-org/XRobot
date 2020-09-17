#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------- */
#include <cmsis_os2.h>
#include <stdint.h>
#include <stdbool.h>

#include "component\ahrs.h"
#include "device\device.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
typedef struct {
  osThreadId_t thread_alert;

  AHRS_Magn_t magn;

  struct {
    int8_t magn_offset[3];
    int8_t magn_scale[3];
  } cali;
} IST8310_t;

/* Exported functions prototypes -------------------------------------------- */
int8_t IST8310_Init(IST8310_t *ist8310);
int8_t IST8310_Restart(void);

bool ST8310_WaitNew(uint32_t timeout);
int8_t ST8310_StartDmaRecv();
uint32_t ST8310_WaitDmaCplt();
int8_t IST8310_Parse(IST8310_t *ist8310);

#ifdef __cplusplus
}
#endif
