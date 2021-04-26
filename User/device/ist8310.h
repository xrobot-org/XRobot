#pragma once

/* Includes ----------------------------------------------------------------- */
#include <cmsis_os2.h>
#include <stdbool.h>
#include <stdint.h>

#include "component/ahrs.h"
#include "device/device.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
typedef struct {
  struct {
    float x;
    float y;
    float z;
  } magn_offset; /* 磁力计偏置 */

  struct {
    float x;
    float y;
    float z;
  } magn_scale;   /* 磁力计缩放 */
} IST8310_Cali_t; /* IST8310校准数据 */

typedef struct {
  AHRS_Magn_t magn;
  const IST8310_Cali_t *cali;
} IST8310_t;

/* Exported functions prototypes -------------------------------------------- */
int8_t IST8310_Init(IST8310_t *ist8310, const IST8310_Cali_t *cali);
int8_t IST8310_Restart(void);

bool IST8310_WaitNew(uint32_t timeout);
int8_t IST8310_StartDmaRecv();
uint32_t IST8310_WaitDmaCplt();
int8_t IST8310_Parse(IST8310_t *ist8310);
