#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "comp_ahrs.h"
#include "dev.h"
#include "semphr.h"

typedef struct {
  Vector3_t gyro_offset; /* 陀螺仪偏置 */
} BMI088_Cali_t;         /* BMI088校准数据 */

typedef struct {
  struct {
    SemaphoreHandle_t gyro_new;
    SemaphoreHandle_t accl_new;
    SemaphoreHandle_t gyro_raw;
    SemaphoreHandle_t accl_raw;
  } sem;

  Vector3_t accl;
  Vector3_t gyro;

  float temp; /* 温度 */

  const BMI088_Cali_t *cali;
} BMI088_t;

int8_t BMI088_Init(BMI088_t *bmi088, const BMI088_Cali_t *cali);
int8_t BMI088_Restart(void);

bool BMI088_GyroStable(Vector3_t *gyro);

/* Sensor use right-handed coordinate system. */
/*
                x < R(logo)
                        y
                UP is z
        All implementation should follow this rule.
 */
bool BMI088_AcclWaitNew(BMI088_t *bmi088, uint32_t timeout);
bool BMI088_GyroWaitNew(BMI088_t *bmi088, uint32_t timeout);

/*
  BMI088的Accl和Gyro共用同一个DMA通道，所以一次只能读一个传感器。
  即BMI088_AcclStartDmaRecv() 和 BMI088_AcclWaitDmaCplt() 中间不能
  出现 BMI088_GyroStartDmaRecv()。
*/
int8_t BMI088_AcclStartDmaRecv();
int8_t BMI088_AcclWaitDmaCplt(BMI088_t *bmi088);
int8_t BMI088_GyroStartDmaRecv();
int8_t BMI088_GyroWaitDmaCplt(BMI088_t *bmi088);
int8_t BMI088_ParseAccl(BMI088_t *bmi088);
int8_t BMI088_ParseGyro(BMI088_t *bmi088);
float BMI088_GetUpdateFreq(BMI088_t *bmi088);
