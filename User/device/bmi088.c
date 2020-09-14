/*
        BMI088 陀螺仪+加速度计传感器。

*/

/* Includes ------------------------------------------------------------------*/
#include "bmi088.h"

#include <cmsis_os2.h>
#include <gpio.h>
#include <stdbool.h>
#include <string.h>

#include "bsp\delay.h"
#include "bsp\gpio.h"
#include "bsp\spi.h"
#include "component\user_math.h"

/* Private define ------------------------------------------------------------*/
#define BMI088_REG_ACCL_CHIP_ID (0x00)
#define BMI088_REG_ACCL_ERR (0x02)
#define BMI088_REG_ACCL_STATUS (0x03)
#define BMI088_REG_ACCL_X_LSB (0x12)
#define BMI088_REG_ACCL_X_MSB (0x13)
#define BMI088_REG_ACCL_Y_LSB (0x14)
#define BMI088_REG_ACCL_Y_MSB (0x15)
#define BMI088_REG_ACCL_Z_LSB (0x16)
#define BMI088_REG_ACCL_Z_MSB (0x17)
#define BMI088_REG_ACCL_SENSORTIME_0 (0x18)
#define BMI088_REG_ACCL_SENSORTIME_1 (0x19)
#define BMI088_REG_ACCL_SENSORTIME_2 (0x1A)
#define BMI088_REG_ACCL_INT_STAT_1 (0x1D)
#define BMI088_REG_ACCL_TEMP_MSB (0x22)
#define BMI088_REG_ACCL_TEMP_LSB (0x23)
#define BMI088_REG_ACCL_CONF (0x40)
#define BMI088_REG_ACCL_RANGE (0x41)
#define BMI088_REG_ACCL_INT1_IO_CONF (0x53)
#define BMI088_REG_ACCL_INT2_IO_CONF (0x54)
#define BMI088_REG_ACCL_INT1_INT2_MAP_DATA (0x58)
#define BMI088_REG_ACCL_SELF_TEST (0x6D)
#define BMI088_REG_ACCL_PWR_CONF (0x7C)
#define BMI088_REG_ACCL_PWR_CTRL (0x7D)
#define BMI088_REG_ACCL_SOFTRESET (0x7E)

#define BMI088_REG_GYRO_CHIP_ID (0x00)
#define BMI088_REG_GYRO_X_LSB (0x02)
#define BMI088_REG_GYRO_X_MSB (0x03)
#define BMI088_REG_GYRO_Y_LSB (0x04)
#define BMI088_REG_GYRO_Y_MSB (0x05)
#define BMI088_REG_GYRO_Z_LSB (0x06)
#define BMI088_REG_GYRO_Z_MSB (0x07)
#define BMI088_REG_GYRO_INT_STAT_1 (0x0A)
#define BMI088_REG_GYRO_RANGE (0x0F)
#define BMI088_REG_GYRO_BANDWIDTH (0x10)
#define BMI088_REG_GYRO_LPM1 (0x11)
#define BMI088_REG_GYRO_SOFTRESET (0x14)
#define BMI088_REG_GYRO_INT_CTRL (0x15)
#define BMI088_REG_GYRO_INT3_INT4_IO_CONF (0x16)
#define BMI088_REG_GYRO_INT3_INT4_IO_MAP (0x18)
#define BMI088_REG_GYRO_SELF_TEST (0x3C)

#define BMI088_CHIP_ID_ACCL (0x1E)
#define BMI088_CHIP_ID_GYRO (0x0F)

#define BMI088_LEN_RX_BUFF (19)
/* Private macro -------------------------------------------------------------*/
#define BMI088_ACCL_NSS_SET() \
  HAL_GPIO_WritePin(ACCL_CS_GPIO_Port, ACCL_CS_Pin, GPIO_PIN_SET)
#define BMI088_ACCL_NSS_RESET() \
  HAL_GPIO_WritePin(ACCL_CS_GPIO_Port, ACCL_CS_Pin, GPIO_PIN_RESET)

#define BMI088_GYRO_NSS_SET() \
  HAL_GPIO_WritePin(GYRO_CS_GPIO_Port, GYRO_CS_Pin, GPIO_PIN_SET)
#define BMI088_GYRO_NSS_RESET() \
  HAL_GPIO_WritePin(GYRO_CS_GPIO_Port, GYRO_CS_Pin, GPIO_PIN_RESET)

/* Private typedef -----------------------------------------------------------*/
typedef enum {
  BMI_ACCL,
  BMI_GYRO,
} BMI_Device_t;

/* Private variables ---------------------------------------------------------*/
static uint8_t buffer[2];
static uint8_t bmi088_rxbuf[BMI088_LEN_RX_BUFF];

static osThreadId_t thread_alert;
static bool inited = false;

/* Private function  ---------------------------------------------------------*/
static void BMI_WriteSingle(BMI_Device_t dv, uint8_t reg, uint8_t data) {
  buffer[0] = (reg & 0x7f);
  buffer[1] = data;

  BSP_Delay(1);
  switch (dv) {
    case BMI_ACCL:
      BMI088_ACCL_NSS_RESET();
      break;

    case BMI_GYRO:
      BMI088_GYRO_NSS_RESET();
      break;
  }

  HAL_SPI_Transmit(BSP_SPI_GetHandle(BSP_SPI_IMU), buffer, 2u, 20u);

  switch (dv) {
    case BMI_ACCL:
      BMI088_ACCL_NSS_SET();
      break;

    case BMI_GYRO:
      BMI088_GYRO_NSS_SET();
      break;
  }
}

static uint8_t BMI_ReadSingle(BMI_Device_t dv, uint8_t reg) {
  BSP_Delay(1);
  switch (dv) {
    case BMI_ACCL:
      BMI088_ACCL_NSS_RESET();
      break;

    case BMI_GYRO:
      BMI088_GYRO_NSS_RESET();
      break;
  }
  buffer[0] = (uint8_t)(reg | 0x80);
  HAL_SPI_Transmit(BSP_SPI_GetHandle(BSP_SPI_IMU), buffer, 1u, 20u);
  HAL_SPI_Receive(BSP_SPI_GetHandle(BSP_SPI_IMU), buffer, 2u, 20u);

  switch (dv) {
    case BMI_ACCL:
      BMI088_ACCL_NSS_SET();
      return buffer[1];

    case BMI_GYRO:
      BMI088_GYRO_NSS_SET();
      return buffer[0];
  }
}

static void BMI_Read(BMI_Device_t dv, uint8_t reg, uint8_t *data, uint8_t len) {
  if (data == NULL) return;

  switch (dv) {
    case BMI_ACCL:
      BMI088_ACCL_NSS_RESET();
      break;

    case BMI_GYRO:
      BMI088_GYRO_NSS_RESET();
      break;
  }
  buffer[0] = (uint8_t)(reg | 0x80);
  HAL_SPI_Transmit(BSP_SPI_GetHandle(BSP_SPI_IMU), buffer, 1u, 20u);
  HAL_SPI_Receive_DMA(BSP_SPI_GetHandle(BSP_SPI_IMU), data, len);
}

static void BMI088_RxCpltCallback(void) {
  if (HAL_GPIO_ReadPin(ACCL_CS_GPIO_Port, ACCL_CS_Pin) == GPIO_PIN_RESET) {
    BMI088_ACCL_NSS_SET();
    osThreadFlagsSet(thread_alert, SIGNAL_BMI088_ACCL_RAW_REDY);
  }
  if (HAL_GPIO_ReadPin(GYRO_CS_GPIO_Port, GYRO_CS_Pin) == GPIO_PIN_RESET) {
    BMI088_GYRO_NSS_SET();
    osThreadFlagsSet(thread_alert, SIGNAL_BMI088_GYRO_RAW_REDY);
  }
}

static void BMI088_AcclIntCallback(void) {
  osThreadFlagsSet(thread_alert, SIGNAL_BMI088_ACCL_NEW_DATA);
}

static void BMI088_GyroIntCallback(void) {
  osThreadFlagsSet(thread_alert, SIGNAL_BMI088_GYRO_NEW_DATA);
}

/* Exported functions --------------------------------------------------------*/
int8_t BMI088_Init(BMI088_t *bmi088) {
  if (bmi088 == NULL) return DEVICE_ERR_NULL;

  if (inited) return DEVICE_ERR_INITED;

  if ((thread_alert = osThreadGetId()) == NULL) return DEVICE_ERR_NULL;

  BMI_WriteSingle(BMI_ACCL, BMI088_REG_ACCL_SOFTRESET, 0xB6);
  BMI_WriteSingle(BMI_GYRO, BMI088_REG_GYRO_SOFTRESET, 0xB6);
  BSP_Delay(30);

  /* Switch accl to SPI mode. */
  BMI_ReadSingle(BMI_ACCL, BMI088_CHIP_ID_ACCL);

  if (BMI_ReadSingle(BMI_ACCL, BMI088_REG_ACCL_CHIP_ID) != BMI088_CHIP_ID_ACCL)
    return DEVICE_ERR_NO_DEV;

  if (BMI_ReadSingle(BMI_GYRO, BMI088_REG_GYRO_CHIP_ID) != BMI088_CHIP_ID_GYRO)
    return DEVICE_ERR_NO_DEV;

  BSP_GPIO_DisableIRQ(ACCL_INT_Pin);
  BSP_GPIO_DisableIRQ(GYRO_INT_Pin);

  BSP_SPI_RegisterCallback(BSP_SPI_IMU, BSP_SPI_RX_CPLT_CB,
                           BMI088_RxCpltCallback);
  BSP_GPIO_RegisterCallback(ACCL_INT_Pin, BMI088_AcclIntCallback);
  BSP_GPIO_RegisterCallback(GYRO_INT_Pin, BMI088_GyroIntCallback);

  /* Accl init. */
  /* Filter setting: Normal. */
  /* ODR: 0xAA: 400Hz. 0xA9: 200Hz. 0xA8: 100Hz. 0xA6: 25Hz. */
  BMI_WriteSingle(BMI_ACCL, BMI088_REG_ACCL_CONF, 0xA8);

  /* 0x00: +-3G. 0x01: +-6G. 0x02: +-12G. 0x03: +-24G. */
  BMI_WriteSingle(BMI_ACCL, BMI088_REG_ACCL_RANGE, 0x01);

  /* INT1 as output. Push-pull. Active low. Output. */
  BMI_WriteSingle(BMI_ACCL, BMI088_REG_ACCL_INT1_IO_CONF, 0x08);

  /* Map data ready interrupt to INT1. */
  BMI_WriteSingle(BMI_ACCL, BMI088_REG_ACCL_INT1_INT2_MAP_DATA, 0x04);

  /* Turn on accl. Now we can read data. */
  BMI_WriteSingle(BMI_ACCL, BMI088_REG_ACCL_PWR_CTRL, 0x04);
  BSP_Delay(50);

  /* Gyro init. */
  /* 0x00: +-2000. 0x01: +-1000. 0x02: +-500. 0x03: +-250. 0x04: +-125. */
  BMI_WriteSingle(BMI_GYRO, BMI088_REG_GYRO_RANGE, 0x01);

  /* Filter bw: 47Hz. */
  /* ODR: 0x03: 400Hz. 0x06: 200Hz. 0x07: 100Hz. */
  BMI_WriteSingle(BMI_GYRO, BMI088_REG_GYRO_BANDWIDTH, 0x07);

  /* INT3 and INT4 as output. Push-pull. Active low. */
  BMI_WriteSingle(BMI_GYRO, BMI088_REG_GYRO_INT3_INT4_IO_CONF, 0x00);

  /* Map data ready interrupt to INT3. */
  BMI_WriteSingle(BMI_GYRO, BMI088_REG_GYRO_INT3_INT4_IO_MAP, 0x01);

  /* Enable new data interrupt. */
  BMI_WriteSingle(BMI_GYRO, BMI088_REG_GYRO_INT_CTRL, 0x80);

  BSP_Delay(10);

  inited = true;

  BSP_GPIO_EnableIRQ(ACCL_INT_Pin);
  BSP_GPIO_EnableIRQ(GYRO_INT_Pin);
  return DEVICE_OK;
}

uint32_t BMI088_WaitNew() {
  return osThreadFlagsWait(
      SIGNAL_BMI088_ACCL_NEW_DATA | SIGNAL_BMI088_GYRO_NEW_DATA, osFlagsWaitAll,
      osWaitForever);
}

int8_t BMI088_AcclStartDmaRecv() {
  BMI_Read(BMI_ACCL, BMI088_REG_ACCL_X_LSB, bmi088_rxbuf, BMI088_LEN_RX_BUFF);
  return DEVICE_OK;
}

uint32_t BMI088_AcclWaitDmaCplt() {
  return osThreadFlagsWait(SIGNAL_BMI088_ACCL_RAW_REDY, osFlagsWaitAll,
                           osWaitForever);
}

int8_t BMI088_GyroStartDmaRecv() {
  BMI_Read(BMI_GYRO, BMI088_REG_GYRO_X_LSB, bmi088_rxbuf + 7, 6u);
  return DEVICE_OK;
}

uint32_t BMI088_GyroWaitDmaCplt() {
  return osThreadFlagsWait(SIGNAL_BMI088_GYRO_RAW_REDY, osFlagsWaitAll,
                           osWaitForever);
}

int8_t BMI088_ParseAccl(BMI088_t *bmi088) {
  if (bmi088 == NULL) return DEVICE_ERR_NULL;

#if 1
  int16_t raw_x, raw_y, raw_z;
  memcpy(&raw_x, bmi088_rxbuf + 1, sizeof(int16_t));
  memcpy(&raw_y, bmi088_rxbuf + 3, sizeof(int16_t));
  memcpy(&raw_z, bmi088_rxbuf + 5, sizeof(int16_t));

  /* 3G: 10920. 6G: 5460. 12G: 2730. 24G: 1365. */
  bmi088->accl.x = (float)raw_x / 5460.f;
  bmi088->accl.y = (float)raw_y / 5460.f;
  bmi088->accl.z = (float)raw_z / 5460.f;

#else
  const int16_t *praw_x = (int16_t *)(bmi088_rxbuf + 1);
  const int16_t *praw_y = (int16_t *)(bmi088_rxbuf + 3);
  const int16_t *praw_z = (int16_t *)(bmi088_rxbuf + 5);

  /* 3G: 10920. 6G: 5460. 12G: 2730. 24G: 1365. */
  bmi088->accl.x = (float)*praw_x / 5460.f;
  bmi088->accl.y = (float)*praw_y / 5460.f;
  bmi088->accl.z = (float)*praw_z / 5460.f;

#endif

  uint16_t raw_temp =
      (uint16_t)((bmi088_rxbuf[17] << 3) | (bmi088_rxbuf[18] >> 5));

  if (raw_temp > 1023) raw_temp -= 2048;

  bmi088->temp = raw_temp * 0.125f + 23.f;

  return DEVICE_OK;
}

int8_t BMI088_ParseGyro(BMI088_t *bmi088) {
  if (bmi088 == NULL) return DEVICE_ERR_NULL;

#if 1
  /* Gyroscope imu_raw -> degrees/sec -> radians/sec */
  int16_t raw_x, raw_y, raw_z;
  memcpy(&raw_x, bmi088_rxbuf + 7, sizeof(int16_t));
  memcpy(&raw_y, bmi088_rxbuf + 9, sizeof(int16_t));
  memcpy(&raw_z, bmi088_rxbuf + 11, sizeof(int16_t));

  /* FS125: 262.144. FS250: 131.072. FS500: 65.536. FS1000: 32.768.
   * FS2000: 16.384.*/
  /* 3G: 10920. 6G: 5460. 12G: 2730. 24G: 1365. */
  bmi088->gyro.x = (float)raw_x / 32.768f * MATH_DEG_TO_RAD_MULT;
  bmi088->gyro.y = (float)raw_y / 32.768f * MATH_DEG_TO_RAD_MULT;
  bmi088->gyro.z = (float)raw_z / 32.768f * MATH_DEG_TO_RAD_MULT;

#else
  /* Gyroscope imu_raw -> degrees/sec -> radians/sec */
  const int16_t *raw_x = (int16_t *)(bmi088_rxbuf + 7);
  const int16_t *raw_y = (int16_t *)(bmi088_rxbuf + 9);
  const int16_t *raw_z = (int16_t *)(bmi088_rxbuf + 11);

  /* FS125: 262.144. FS250: 131.072. FS500: 65.536. FS1000: 32.768.
   * FS2000: 16.384.*/
  bmi088->gyro.x = (float)*raw_x / 32.768f * MATH_DEG_TO_RAD_MULT;
  bmi088->gyro.y = (float)*raw_y / 32.768f * MATH_DEG_TO_RAD_MULT;
  bmi088->gyro.z = (float)*raw_z / 32.768f * MATH_DEG_TO_RAD_MULT;
#endif

  return DEVICE_ERR_NULL;
}

float BMI088_GetUpdateFreq(BMI088_t *bmi088) {
  (void)bmi088;
  return 100.f;
}
