#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------- */
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
  } gyro_offset; /* 陀螺仪偏置 */
} BMI088_Cali_t; /* BMI088校准数据 */

typedef struct {
  AHRS_Accl_t accl;
  AHRS_Gyro_t gyro;

  float temp; /* 温度 */

  const BMI088_Cali_t *cali;
} BMI088_t;

/* Exported functions prototypes -------------------------------------------- */
int8_t BMI088_Init(BMI088_t *bmi088, const BMI088_Cali_t *cali);
int8_t BMI088_Restart(void);

bool BMI088_GyroStable(AHRS_Gyro_t *gyro);

/* Sensor use right-handed coordinate system. */
/*
                x < R(logo)
                        y
                UP is z
        All implementation should follow this rule.
 */
uint32_t BMI088_WaitNew();

/*
  BMI088的Accl和Gyro共用同一个DMA通道，所以一次只能读一个传感器。
  即BMI088_AcclStartDmaRecv() 和 BMI088_AcclWaitDmaCplt() 中间不能
  出现 BMI088_GyroStartDmaRecv()。
*/
int8_t BMI088_AcclStartDmaRecv();
uint32_t BMI088_AcclWaitDmaCplt();
int8_t BMI088_GyroStartDmaRecv();
uint32_t BMI088_GyroWaitDmaCplt();
int8_t BMI088_ParseAccl(BMI088_t *bmi088);
int8_t BMI088_ParseGyro(BMI088_t *bmi088);
float BMI088_GetUpdateFreq(BMI088_t *bmi088);

#ifdef __cplusplus
}
#endif
