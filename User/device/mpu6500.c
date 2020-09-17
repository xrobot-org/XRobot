/*
        MPU6500 + IST8310 组合惯性测量单元。

*/

/* Includes ----------------------------------------------------------------- */
#include "imu.h"

/* Include 标准库 */
#include <stdbool.h>
#include <string.h>

/* Include BSP相关的头文件 */
#include "bsp_delay.h"
#include "bsp_spi.h"

/* Include Component相关的头文件 */
#include "user_math.h"

/* Private define ----------------------------------------------------------- */
#define MPU6500_SELF_TEST_XG (0x00)
#define MPU6500_SELF_TEST_YG (0x01)
#define MPU6500_SELF_TEST_ZG (0x02)
#define MPU6500_SELF_TEST_XA (0x0D)
#define MPU6500_SELF_TEST_YA (0x0E)
#define MPU6500_SEFLF_TEST_ZA (0x0F)
#define MPU6500_XG_OFFSET_H (0x13)
#define MPU6500_XG_OFFSET_L (0x14)
#define MPU6500_YG_OFFSET_H (0x15)
#define MPU6500_YG_OFFSET_L (0x16)
#define MPU6500_ZG_OFFSET_H (0x17)
#define MPU6500_ZG_OFFSET_L (0x18)
#define MPU6500_SMPLRT_DIV (0x19)
#define MPU6500_CONFIG (0x1A)
#define MPU6500_GYRO_CONFIG (0x1B)
#define MPU6500_ACCEL_CONFIG (0x1C)
#define MPU6500_ACCEL_CONFIG_2 (0x1D)
#define MPU6500_LP_ACCEL_ODR (0x1E)
#define MPU6500_MOT_THR (0x1F)
#define MPU6500_FIFO_EN (0x23)
#define MPU6500_I2C_MST_CTRL (0x24)
#define MPU6500_I2C_SLV0_ADDR (0x25)
#define MPU6500_I2C_SLV0_REG (0x26)
#define MPU6500_I2C_SLV0_CTRL (0x27)
#define MPU6500_I2C_SLV1_ADDR (0x28)
#define MPU6500_I2C_SLV1_REG (0x29)
#define MPU6500_I2C_SLV1_CTRL (0x2A)
#define MPU6500_I2C_SLV2_ADDR (0x2B)
#define MPU6500_I2C_SLV2_REG (0x2C)
#define MPU6500_I2C_SLV2_CTRL (0x2D)
#define MPU6500_I2C_SLV3_ADDR (0x2E)
#define MPU6500_I2C_SLV3_REG (0x2F)
#define MPU6500_I2C_SLV3_CTRL (0x30)
#define MPU6500_I2C_SLV4_ADDR (0x31)
#define MPU6500_I2C_SLV4_REG (0x32)
#define MPU6500_I2C_SLV4_DO (0x33)
#define MPU6500_I2C_SLV4_CTRL (0x34)
#define MPU6500_I2C_SLV4_DI (0x35)
#define MPU6500_I2C_MST_STATUS (0x36)
#define MPU6500_INT_PIN_CFG (0x37)
#define MPU6500_INT_ENABLE (0x38)
#define MPU6500_INT_STATUS (0x3A)
#define MPU6500_ACCEL_XOUT_H (0x3B)
#define MPU6500_ACCEL_XOUT_L (0x3C)
#define MPU6500_ACCEL_YOUT_H (0x3D)
#define MPU6500_ACCEL_YOUT_L (0x3E)
#define MPU6500_ACCEL_ZOUT_H (0x3F)
#define MPU6500_ACCEL_ZOUT_L (0x40)
#define MPU6500_TEMP_OUT_H (0x41)
#define MPU6500_TEMP_OUT_L (0x42)
#define MPU6500_GYRO_XOUT_H (0x43)
#define MPU6500_GYRO_XOUT_L (0x44)
#define MPU6500_GYRO_YOUT_H (0x45)
#define MPU6500_GYRO_YOUT_L (0x46)
#define MPU6500_GYRO_ZOUT_H (0x47)
#define MPU6500_GYRO_ZOUT_L (0x48)
#define MPU6500_EXT_SENS_DATA_00 (0x49)
#define MPU6500_EXT_SENS_DATA_01 (0x4A)
#define MPU6500_EXT_SENS_DATA_02 (0x4B)
#define MPU6500_EXT_SENS_DATA_03 (0x4C)
#define MPU6500_EXT_SENS_DATA_04 (0x4D)
#define MPU6500_EXT_SENS_DATA_05 (0x4E)
#define MPU6500_EXT_SENS_DATA_06 (0x4F)
#define MPU6500_EXT_SENS_DATA_07 (0x50)
#define MPU6500_EXT_SENS_DATA_08 (0x51)
#define MPU6500_EXT_SENS_DATA_09 (0x52)
#define MPU6500_EXT_SENS_DATA_10 (0x53)
#define MPU6500_EXT_SENS_DATA_11 (0x54)
#define MPU6500_EXT_SENS_DATA_12 (0x55)
#define MPU6500_EXT_SENS_DATA_13 (0x56)
#define MPU6500_EXT_SENS_DATA_14 (0x57)
#define MPU6500_EXT_SENS_DATA_15 (0x58)
#define MPU6500_EXT_SENS_DATA_16 (0x59)
#define MPU6500_EXT_SENS_DATA_17 (0x5A)
#define MPU6500_EXT_SENS_DATA_18 (0x5B)
#define MPU6500_EXT_SENS_DATA_19 (0x5C)
#define MPU6500_EXT_SENS_DATA_20 (0x5D)
#define MPU6500_EXT_SENS_DATA_21 (0x5E)
#define MPU6500_EXT_SENS_DATA_22 (0x5F)
#define MPU6500_EXT_SENS_DATA_23 (0x60)
#define MPU6500_I2C_SLV0_DO (0x63)
#define MPU6500_I2C_SLV1_DO (0x64)
#define MPU6500_I2C_SLV2_DO (0x65)
#define MPU6500_I2C_SLV3_DO (0x66)
#define MPU6500_I2C_MST_DELAY_CTRL (0x67)
#define MPU6500_SIGNAL_PATH_RESET (0x68)
#define MPU6500_MOT_DETECT_CTRL (0x69)
#define MPU6500_USER_CTRL (0x6A)
#define MPU6500_PWR_MGMT_1 (0x6B)
#define MPU6500_PWR_MGMT_2 (0x6C)
#define MPU6500_FIFO_COUNTH (0x72)
#define MPU6500_FIFO_COUNTL (0x73)
#define MPU6500_FIFO_R_W (0x74)
#define MPU6500_WHO_AM_I (0x75)
#define MPU6500_XA_OFFSET_H (0x77)
#define MPU6500_XA_OFFSET_L (0x78)
#define MPU6500_YA_OFFSET_H (0x7A)
#define MPU6500_YA_OFFSET_L (0x7B)
#define MPU6500_ZA_OFFSET_H (0x7D)
#define MPU6500_ZA_OFFSET_L (0x7E)

#define MPU6500_ID (0x70)

/* Private macro ------------------------------------------------------------ */
/* Private typedef ---------------------------------------------------------- */
__packed struct {
  __packed struct {
    int16_t x;
    int16_t y;
    int16_t z;
  } accl;

  int16_t temp;

  __packed struct {
    int16_t x;
    int16_t y;
    int16_t z;
  } gyro;

  __packed struct {
    int16_t x;
    int16_t y;
    int16_t z;
  } magn;
} packed;
/* Private variables -------------------------------------------------------- */
static uint8_t buffer[2];
static IMU_t *gimu;
static bool inited = false;

/* Private function  -------------------------------------------------------- */
void IMU_RxCpltCallback(void) {
  osThreadFlagsSet(gimu->received_alert,
                   IMU_SIGNAL_RAW_ACCL_REDY | IMU_SIGNAL_RAW_GYRO_REDY);
}

void IMU_ExtIntCallback(void) { IMU_StartReceiving(gimu); }

static void MPU_WriteSingle(uint8_t reg, uint8_t data) {
  buffer[0] = (reg & 0x7f);
  buffer[1] = data;

  BSP_SPI_Transmit(BSP_SPI_IMU, buffer, 2);

  BSP_Delay(5);
}

static uint8_t MPU_ReadSingle(uint8_t reg) {
  buffer[0] = (reg | 0x80);
  BSP_SPI_Transmit(BSP_SPI_IMU, buffer, 1);
  BSP_SPI_Receive(BSP_SPI_IMU, buffer, 1);

  return buffer[0];
}

static void MPU_Read(uint8_t reg, uint8_t *data, uint8_t len) {
  if (data == NULL) return;

  buffer[0] = (reg | 0x80);

  BSP_SPI_Transmit(BSP_SPI_IMU, buffer, 1);
  BSP_SPI_Receive(BSP_SPI_IMU, data, len);
}

/* Exported functions ------------------------------------------------------- */
int IMU_Init(IMU_t *imu) {
  if (imu == NULL) return -1;

  if (inited) return -1;

  if (MPU_ReadSingle(MPU6500_WHO_AM_I) != MPU6500_ID) return -1;

  gimu = imu;

  /* MPU6500 init. */
  MPU_WriteSingle(MPU6500_PWR_MGMT_1, 0x80); /* Reset device */
  BSP_Delay(100);
  MPU_WriteSingle(MPU6500_SIGNAL_PATH_RESET, 0x0f); /* Reset device */
  BSP_Delay(100);

  MPU_WriteSingle(MPU6500_PWR_MGMT_1, 0x03); /* Clock source -> gyro-z */
  MPU_WriteSingle(MPU6500_PWR_MGMT_2, 0x00); /* Enable acc & gyro */
  MPU_WriteSingle(MPU6500_CONFIG, 0x04);     /* Disable acc LPF */
  MPU_WriteSingle(MPU6500_GYRO_CONFIG,
                  0x18); /* gyro range 0x10:+-1000dps 0x18:+-2000dps */
  MPU_WriteSingle(MPU6500_ACCEL_CONFIG, 0x10);   /* +-8G */
  MPU_WriteSingle(MPU6500_ACCEL_CONFIG_2, 0x10); /* Disable acc LPF */

  BSP_Delay(10);

  BSP_SPI_RegisterCallback(BSP_SPI_IMU, BSP_SPI_RX_COMPLETE_CB,
                           IMU_RxCpltCallback);

  inited = true;
  return 0;
}

IMU_t *IMU_GetDevice(void) {
  if (inited) {
    return gimu;
  }
  return NULL;
}

int IMU_StartReceiving(IMU_t *imu) {
  MPU_Read(MPU6500_ACCEL_XOUT_H, (uint8_t *)&(imu->raw), 20);
  return 0;
}

/* magn_scale[3] is initially zero. So data from uncalibrated magnentmeter is
 * ignored. */
/* Sensor use right-handed coordinate system. */
/*
 *	 x < R (RoboMaster trademark)
 *		 y
 *	 UP is z
 */
int IMU_Parse(IMU_t *imu) {
  if (imu == NULL) return -1;

  uint8_t raw[20];

  memcpy(raw, &(imu->raw), sizeof(imu->raw));

  for (uint8_t i = 0; i < 20; i += 2)
    ((int16_t *)&imu->raw)[i / 2] = (raw[i] << 8) | raw[i + 1];

  imu->data.temp = 21.f + (float)imu->raw.temp / 333.87f;

  /* TODO: Strage accl data. Potential sensor damage. */
  imu->data.accl.x = (float)imu->raw.accl.x / 4096.f;
  imu->data.accl.y = (float)imu->raw.accl.y / 4096.f;
  imu->data.accl.z = (float)imu->raw.accl.z / 4096.f;

  /* Convert gyroscope imu_raw to degrees/sec, then, to radians/sec */
  imu->data.gyro.x = (float)(imu->raw.gyro.x - imu->cali.gyro_offset[0]) /
                     16.384f / 180.f * M_PI;
  imu->data.gyro.y = (float)(imu->raw.gyro.y - imu->cali.gyro_offset[1]) /
                     16.384f / 180.f * M_PI;
  imu->data.gyro.z = (float)(imu->raw.gyro.z - imu->cali.gyro_offset[2]) /
                     16.384f / 180.f * M_PI;

#if 0
	imu->data.magn.x = (float)((imu->raw.magn.x - imu->cali.magn_offset[0]) * imu->cali.magn_scale[0]);
	imu->data.magn.y = (float)((imu->raw.magn.y - imu->cali.magn_offset[1]) * imu->cali.magn_scale[1]);
	imu->data.magn.z = (float)((imu->raw.magn.z - imu->cali.magn_offset[2]) * imu->cali.magn_scale[2]);
#else
  imu->data.magn.x = 0.f;
  imu->data.magn.y = 0.f;
  imu->data.magn.z = 0.f;
#endif
  return 0;
}
