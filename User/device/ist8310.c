/*
        IST8310 地磁传感器。

*/

/* Includes ----------------------------------------------------------------- */
#include "ist8310.h"

#include <gpio.h>
#include <stdbool.h>
#include <string.h>

#include "bsp\delay.h"
#include "bsp\gpio.h"
#include "bsp\i2c.h"

/* Private define ----------------------------------------------------------- */
#define IST8310_WAI (0x00)
#define IST8310_STAT1 (0x02)
#define IST8310_DATAXL (0x03)
#define IST8310_STAT2 (0x09)
#define IST8310_CNTL1 (0x0A)
#define IST8310_CNTL2 (0x0B)
#define IST8310_STR (0x0C)
#define IST8310_TEMPL (0x1C)
#define IST8310_TEMPH (0x1D)
#define IST8310_AVGCNTL (0x41)
#define IST8310_PDCNTL (0x42)

#define IST8310_CHIP_ID (0x10)

#define IST8310_IIC_ADDRESS (0x0E << 1)

#define IST8310_LEN_RX_BUFF (6)
/* Private macro ------------------------------------------------------------ */
#define IST8310_SET() \
  HAL_GPIO_WritePin(CMPS_RST_GPIO_Port, CMPS_RST_Pin, GPIO_PIN_SET)
#define IST8310_RESET() \
  HAL_GPIO_WritePin(CMPS_RST_GPIO_Port, CMPS_RST_Pin, GPIO_PIN_RESET)

/* Private typedef ---------------------------------------------------------- */
/* Private variables -------------------------------------------------------- */
uint8_t ist8310_rxbuf[IST8310_LEN_RX_BUFF];  // TODO: Add static when release

static osThreadId_t thread_alert;
static bool inited = false;

/* Private function  -------------------------------------------------------- */
static void IST8310_WriteSingle(uint8_t reg, uint8_t data) {
  HAL_I2C_Mem_Write(BSP_I2C_GetHandle(BSP_I2C_COMP), IST8310_IIC_ADDRESS, reg,
                    I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

static uint8_t IST8310_ReadSingle(uint8_t reg) {
  uint8_t buf = 0;
  HAL_I2C_Mem_Read(BSP_I2C_GetHandle(BSP_I2C_COMP), IST8310_IIC_ADDRESS, reg,
                   I2C_MEMADD_SIZE_8BIT, &buf, 1, 100);
  return buf;
}

static void IST8310_Read(uint8_t reg, uint8_t *data, uint8_t len) {
  if (data == NULL) return;

  HAL_I2C_Mem_Read_DMA(BSP_I2C_GetHandle(BSP_I2C_COMP), IST8310_IIC_ADDRESS,
                       reg, I2C_MEMADD_SIZE_8BIT, data, len);
}

static void IST8310_Write(uint8_t reg, uint8_t *data, uint8_t len) {
  if (data == NULL) return;

  HAL_I2C_Mem_Write_DMA(BSP_I2C_GetHandle(BSP_I2C_COMP), IST8310_IIC_ADDRESS,
                        reg, I2C_MEMADD_SIZE_8BIT, data, len);
}

static void IST8310_MemRxCpltCallback(void) {
  osThreadFlagsSet(thread_alert, SIGNAL_IST8310_MAGN_RAW_REDY);
}

static void IST8310_IntCallback(void) {
  osThreadFlagsSet(thread_alert, SIGNAL_IST8310_MAGN_NEW_DATA);
}

/* Exported functions ------------------------------------------------------- */
int8_t IST8310_Init(IST8310_t *ist8310) {
  if (ist8310 == NULL) return DEVICE_ERR_NULL;

  if (inited) return DEVICE_ERR_INITED;

  if ((thread_alert = osThreadGetId()) == NULL) return DEVICE_ERR_NULL;

  IST8310_RESET();
  BSP_Delay(50);
  IST8310_SET();
  BSP_Delay(50);

  if (IST8310_ReadSingle(IST8310_WAI) != IST8310_CHIP_ID)
    return DEVICE_ERR_NO_DEV;

  BSP_GPIO_DisableIRQ(CMPS_INT_Pin);

  BSP_I2C_RegisterCallback(BSP_I2C_COMP, HAL_I2C_MEM_RX_CPLT_CB,
                           IST8310_MemRxCpltCallback);
  BSP_GPIO_RegisterCallback(CMPS_INT_Pin, IST8310_IntCallback);

  /* Init. */
  /* 0x00: Stand-By mode. 0x01: Single measurement mode. */

  /* 0x08: Data ready function enable. DRDY signal active low*/
  IST8310_WriteSingle(IST8310_CNTL2, 0x08);

  IST8310_WriteSingle(IST8310_AVGCNTL, 0x02);
  IST8310_WriteSingle(IST8310_PDCNTL, 0xC0);
  BSP_Delay(10);

  inited = true;

  BSP_GPIO_EnableIRQ(CMPS_INT_Pin);
  return DEVICE_OK;
}

bool ST8310_WaitNew(uint32_t timeout) {
  uint8_t data = 1;
  IST8310_Write(IST8310_CNTL1, &data, 1);

  if (osThreadFlagsWait(SIGNAL_IST8310_MAGN_NEW_DATA, osFlagsWaitAny,
                        timeout) != 0) {
    return false;
  }

  return true;
}

int8_t ST8310_StartDmaRecv() {
  IST8310_Read(IST8310_DATAXL, ist8310_rxbuf, IST8310_LEN_RX_BUFF);
  return DEVICE_OK;
}

uint32_t ST8310_WaitDmaCplt() {
  return osThreadFlagsWait(SIGNAL_BMI088_ACCL_RAW_REDY, osFlagsWaitAll,
                           osWaitForever);
}

int8_t IST8310_Parse(IST8310_t *ist8310) {
  if (ist8310 == NULL) return DEVICE_ERR_NULL;

#if 1
  /* Gyroscope imu_raw -> degrees/sec -> radians/sec */
  int16_t raw_x, raw_y, raw_z;
  memcpy(&raw_x, ist8310_rxbuf + 0, sizeof(int16_t));
  memcpy(&raw_y, ist8310_rxbuf + 2, sizeof(int16_t));
  memcpy(&raw_z, ist8310_rxbuf + 4, sizeof(int16_t));

  ist8310->magn.x = (float)raw_x * 3.f / 20.f;
  ist8310->magn.y = (float)raw_y * 3.f / 20.f;
  ist8310->magn.z = (float)raw_z * 3.f / 20.f;

#else
  const int16_t *raw_x = (int16_t *)(ist8310_rxbuf + 0);
  const int16_t *raw_y = (int16_t *)(ist8310_rxbuf + 2);
  const int16_t *raw_z = (int16_t *)(ist8310_rxbuf + 4);

  ist8310->magn.x = (float)*raw_x * 3.f / 20.f;
  ist8310->magn.y = (float)*raw_y * 3.f / 20.f;
  ist8310->magn.z = (float)*raw_z * 3.f / 20.f;
#endif

  return DEVICE_OK;
}
