#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

#include "component\ahrs.h"
#include "device\device.h"

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct {
  AHRS_Accl_t accl;
  AHRS_Gyro_t gyro;

  float temp;

  struct {
    int8_t gyro_offset[3];
  } cali;
} BMI088_t;

/* Exported functions prototypes ---------------------------------------------*/
int8_t BMI088_Init(BMI088_t *bmi088);
int8_t BMI088_Restart(void);

/* Sensor use right-handed coordinate system. */
/*
                x < R(logo)
                        y
                UP is z
        All implementation should follow this rule.
 */
uint32_t BMI088_WaitNew();
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
