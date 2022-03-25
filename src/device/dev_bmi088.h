#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "comp_ahrs.h"
#include "dev.h"
#include "semphr.h"

typedef struct {
  vector3_t gyro_offset; /* 陀螺仪偏置 */
} bmi088_cali_t;         /* BMI088校准数据 */

typedef struct {
  bool inverted;
} bmi088_param_t;

typedef struct {
  struct {
    SemaphoreHandle_t new;
    SemaphoreHandle_t gyro_new;
    SemaphoreHandle_t accl_new;
    SemaphoreHandle_t gyro_raw;
    SemaphoreHandle_t accl_raw;
  } sem;

  vector3_t accl;
  vector3_t gyro;

  float temp; /* 温度 */

  const bmi088_cali_t *cali;

  const bmi088_param_t *param;
} bmi088_t;

int8_t bmi088_init(bmi088_t *bmi088, const bmi088_cali_t *cali,
                   const bmi088_param_t *param);
int8_t bmi088_restart(void);

/* Sensor use right-handed coordinate system. */
/*
                x < R(logo)
                        y
                UP is z
        All implementation should follow this rule.
 */
bool bmi088_wait_new(bmi088_t *bmi088, uint32_t timeout);
bool bmi088_accl_wait_new(bmi088_t *bmi088, uint32_t timeout);
bool bmi088_gyro_wait_new(bmi088_t *bmi088, uint32_t timeout);

/*
  BMI088的Accl和Gyro共用同一个DMA通道，所以一次只能读一个传感器。
  即BMI088_AcclStartDmaRecv() 和 bmi088_accl_wait_dma_cplt() 中间不能
  出现 bmi088_gyro_start_dma_recv()。
*/
int8_t bmi088_accl_start_dma_recv();
int8_t bmi088_accl_wait_dma_cplt(bmi088_t *bmi088);
int8_t bmi088_gyro_start_dma_recv();
int8_t bmi088_gyro_wait_dma_cplt(bmi088_t *bmi088);
int8_t bmi088_parse_accl(bmi088_t *bmi088);
int8_t bmi088_parse_gyro(bmi088_t *bmi088);
float bmi088_get_update_freq(bmi088_t *bmi088);
