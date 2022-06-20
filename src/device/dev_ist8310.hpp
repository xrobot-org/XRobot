#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "comp_ahrs.hpp"
#include "dev.hpp"
#include "semphr.h"
typedef struct {
  vector3_t magn_offset; /* 磁力计偏置 */
  vector3_t magn_scale;  /* 磁力计缩放 */
} ist8310_cali_t;        /* IST8310校准数据 */

typedef struct {
  vector3_t magn;
  const ist8310_cali_t *cali;
  struct {
    SemaphoreHandle_t recv;
    SemaphoreHandle_t _new;
  } sem;
} ist8310_t;

int8_t ist8310_init(ist8310_t *ist8310, const ist8310_cali_t *cali);
int8_t ist8310_restart(void);

bool ist8310_wait_new(ist8310_t *ist8310,uint32_t timeout);
int8_t ist8310_start_dma_recv();
uint32_t ist8310_wait_dma_cplt(ist8310_t *ist8310);
int8_t ist8310_parse(ist8310_t *ist8310);
