/* 
	BMI088 陀螺仪+加速度计传感器。

*/

/* Includes ------------------------------------------------------------------*/
#include "imu.h"

/* Include 标准库 */
#include "stdbool.h"
#include "string.h"

/* Include BSP相关的头文件 */
#include "bsp_delay.h"
#include "bsp_spi.h"
#include "bsp_gpio.h"
#include "gpio.h"

/* Include Component相关的头文件 */
#include "user_math.h"

/* Private define ------------------------------------------------------------*/
#define BMI088_ACCL_CHIP_ID_REG            (0x00)
#define BMI088_ACCL_ERR_REG                (0x02)
#define BMI088_ACCL_STATUS_REG             (0x03)
#define BMI088_ACCL_X_LSB_REG              (0x12)
#define BMI088_ACCL_X_MSB_REG              (0x13)
#define BMI088_ACCL_Y_LSB_REG              (0x14)
#define BMI088_ACCL_Y_MSB_REG              (0x15)
#define BMI088_ACCL_Z_LSB_REG              (0x16)
#define BMI088_ACCL_Z_MSB_REG              (0x17)
#define BMI088_ACCL_SENSORTIME_0_REG       (0x18)
#define BMI088_ACCL_SENSORTIME_1_REG       (0x19)
#define BMI088_ACCL_SENSORTIME_2_REG       (0x1A)
#define BMI088_ACCL_INT_STAT_1_REG         (0x1D)
#define BMI088_TEMP_MSB_REG                (0x22)
#define BMI088_TEMP_LSB_REG                (0x23)
#define BMI088_ACCL_CONF_REG               (0x40)
#define BMI088_ACCL_RANGE_REG              (0x41)
#define BMI088_ACCL_INT1_IO_CONF_REG       (0x53)
#define BMI088_ACCL_INT2_IO_CONF_REG       (0x54)
#define BMI088_ACCL_INT1_INT2_MAP_DATA_REG (0x58)
#define BMI088_ACCL_SELF_TEST_REG          (0x6D)
#define BMI088_ACCL_PWR_CONF_REG           (0x7C)
#define BMI088_ACCL_PWR_CTRL_REG           (0x7D)
#define BMI088_ACCL_SOFTRESET_REG          (0x7E)

#define BMI088_ACCL_CHIP_ID                (0x1E)

#define BMI088_GYRO_CHIP_ID_REG                (0x00)
#define BMI088_GYRO_X_LSB_REG                  (0x02)
#define BMI088_GYRO_X_MSB_REG                  (0x03)
#define BMI088_GYRO_Y_LSB_REG                  (0x04)
#define BMI088_GYRO_Y_MSB_REG                  (0x05)
#define BMI088_GYRO_Z_LSB_REG                  (0x06)
#define BMI088_GYRO_Z_MSB_REG                  (0x07)
#define BMI088_GYRO_INT_STAT_1_REG             (0x0A)
#define BMI088_GYRO_RANGE_REG                  (0x0F)
#define BMI088_GYRO_BANDWIDTH_REG              (0x10)
#define BMI088_GYRO_LPM1_REG                   (0x11)
#define BMI088_GYRO_SOFTRESET_REG              (0x14)
#define BMI088_GYRO_INT_CTRL_REG               (0x15)
#define BMI088_GYRO_INT3_INT4_IO_CONF_REG      (0x16)
#define BMI088_GYRO_INT3_INT4_IO_MAP_REG       (0x18)
#define BMI088_GYRO_SELF_TEST_REG              (0x3C)

#define BMI088_GYRO_CHIP_ID                    (0x0F)

/* Private macro -------------------------------------------------------------*/
#define IMU_ACCL_NSS_Set()		HAL_GPIO_WritePin(ACCL_CS_GPIO_Port, ACCL_CS_Pin, GPIO_PIN_SET)
#define IMU_ACCL_NSS_Reset()	HAL_GPIO_WritePin(ACCL_CS_GPIO_Port, ACCL_CS_Pin, GPIO_PIN_RESET)

#define IMU_GYRO_NSS_Set()		HAL_GPIO_WritePin(GYRO_CS_GPIO_Port, GYRO_CS_Pin, GPIO_PIN_SET)
#define IMU_GYRO_NSS_Reset()	HAL_GPIO_WritePin(GYRO_CS_GPIO_Port, GYRO_CS_Pin, GPIO_PIN_RESET)
		
/* Private typedef -----------------------------------------------------------*/
typedef enum {
	BMI_ACCL,
	BMI_GYRO,
} BMI_Device_t;

/* Private variables ---------------------------------------------------------*/
static uint8_t buffer[2];
static IMU_t *gimu;
static bool inited = false;


/* Private function  ---------------------------------------------------------*/
static void BMI_WriteSingle(BMI_Device_t dv, uint8_t reg, uint8_t data) {
	buffer[0] = (reg  & 0x7f);
	buffer[1] = data;
	
	switch (dv) {
		case BMI_ACCL:
			IMU_ACCL_NSS_Reset();
			break;
		
		case BMI_GYRO:
			IMU_GYRO_NSS_Reset();
			break;
	}
	
	BSP_SPI_Transmit(BSP_SPI_IMU, buffer, 2);
	
	switch (dv) {
		case BMI_ACCL:
			IMU_ACCL_NSS_Set();
			break;
		
		case BMI_GYRO:
			IMU_GYRO_NSS_Set();
			break;
	}
	BSP_Delay(5);
}

static uint8_t BMI_ReadSingle(BMI_Device_t dv, uint8_t reg) {
	buffer[0] = (reg | 0x80);
	
	switch (dv) {
		case BMI_ACCL:
			IMU_ACCL_NSS_Reset();
			break;
		
		case BMI_GYRO:
			IMU_GYRO_NSS_Reset();
			break;
	}
	
	BSP_SPI_Transmit(BSP_SPI_IMU, buffer, 1);
	BSP_SPI_Receive(BSP_SPI_IMU, buffer, 2);
	
	switch (dv) {
		case BMI_ACCL:
			IMU_ACCL_NSS_Reset();
			return buffer[1];
		
		case BMI_GYRO:
			IMU_GYRO_NSS_Reset();
			return buffer[0];
		
		default:
			return 0;
	}
}

static void BMI_Read(BMI_Device_t dv, uint8_t reg, uint8_t *data, uint8_t len) {
	if (data == NULL)
		return;
	
	buffer[0] = (reg | 0x80);
	
	switch (dv) {
		case BMI_ACCL:
			IMU_ACCL_NSS_Reset();
			break;
		
		case BMI_GYRO:
			IMU_GYRO_NSS_Reset();
			break;
	}
	
	switch (dv) {
		case BMI_ACCL:
			/* Read first dummy byte. */
			BSP_SPI_Receive(BSP_SPI_IMU, buffer, 1);
		case BMI_GYRO:
			BSP_SPI_Receive(BSP_SPI_IMU, data, len);
			break;
	}
}

void IMU_RxCpltCallback(void) {
	if (HAL_GPIO_ReadPin(ACCL_CS_GPIO_Port, ACCL_CS_Pin) == GPIO_PIN_RESET) {
		IMU_ACCL_NSS_Set();
		osSignalSet(gimu->received_alert, IMU_SIGNAL_RAW_ACCL_REDY);
	}
	if (HAL_GPIO_ReadPin(GYRO_CS_GPIO_Port, GYRO_CS_Pin) == GPIO_PIN_RESET) {
		IMU_GYRO_NSS_Set();
		osSignalSet(gimu->received_alert, IMU_SIGNAL_RAW_GYRO_REDY);
	}
}

void IMU_AcclIntCallback(void) {
	BMI_Read(BMI_ACCL, BMI088_ACCL_X_LSB_REG, gimu->raw.bytes, 6u);
	/* WTF. Why seperate these 2 reg? */
	//BMI_Read(BMI_ACCL, BMI088_TEMP_MSB_REG, gimu->raw.bytes + 6u, 2u);
	
}

void IMU_GyroIntCallback(void) {
	BMI_Read(BMI_ACCL, BMI088_GYRO_X_LSB_REG, gimu->raw.bytes + 8u, 6u);
}

/* Exported functions --------------------------------------------------------*/
int IMU_Init(IMU_t *imu) {
	if (imu == NULL)
		return IMU_ERR_NULL;
	
	if (inited)
		return IMU_ERR_INITED;
	
	if (BMI_ReadSingle(BMI_ACCL, BMI088_ACCL_CHIP_ID_REG) != BMI088_ACCL_CHIP_ID)
		return IMU_ERR_NO_DEV;
	
	if (BMI_ReadSingle(BMI_GYRO, BMI088_GYRO_CHIP_ID_REG) != BMI088_GYRO_CHIP_ID)
		return IMU_ERR_NO_DEV;
	
	gimu = imu;
	
	/* BMI088 init. */

	BSP_Delay(10);

	BSP_SPI_RegisterCallback(BSP_SPI_IMU, BSP_SPI_RX_COMPLETE_CB, IMU_RxCpltCallback);
	BSP_GPIO_RegisterCallback(ACCL_INT_Pin, IMU_AcclIntCallback);
	BSP_GPIO_RegisterCallback(GYRO_INT_Pin, IMU_GyroIntCallback);
	
	inited = true;
	return IMU_OK;
}

IMU_t *IMU_GetDevice(void) {
	if (inited) {
		return gimu;
	}
	return NULL;
}

int IMU_StartReceiving(IMU_t *imu) {
	//BMI_Read(BMI088_ACCL_XOUT_H, (uint8_t*)&(imu->raw), 20);
	return 0;
}

/* magn_scale[3] is initially zero. So data from uncalibrated magnentmeter is ignored. */
int IMU_Parse(IMU_t *imu) {
	if (imu == NULL)
		return IMU_ERR_NULL;
	
	/* Convert endian. Processing of 4-bytes for one step. */
	for(uint8_t i = 0; i < 5; i++) {
		uint32_t value = (uint32_t)imu->raw.bytes[4 * i];
		*(uint32_t*)(imu->raw.bytes + sizeof(uint32_t)) = (value & 0xFF00FF00) >> 8 | (value & 0x00FF00FF) << 8;
	}

	imu->data.temp = 21.f + (float)imu->raw.packed.temp / 333.87f;
	
	imu->data.accl.x = (float)imu->raw.packed.accl.x;
	imu->data.accl.y = (float)imu->raw.packed.accl.y;
	imu->data.accl.z = (float)imu->raw.packed.accl.z;
	
	/* Convert gyroscope imu_raw to degrees/sec, then, to radians/sec */
	imu->data.gyro.x = (float)(imu->raw.packed.gyro.x - imu->cali.gyro_offset[0]) / 16.384f / 180.f * PI;
	imu->data.gyro.y = (float)(imu->raw.packed.gyro.y - imu->cali.gyro_offset[1]) / 16.384f / 180.f * PI;
	imu->data.gyro.z = (float)(imu->raw.packed.gyro.z - imu->cali.gyro_offset[2]) / 16.384f / 180.f * PI;
	
	imu->data.magn.x = (float)((imu->raw.packed.magn.x - imu->cali.magn_offset[0]) * imu->cali.magn_scale[0]);
	imu->data.magn.y = (float)((imu->raw.packed.magn.y - imu->cali.magn_offset[1]) * imu->cali.magn_scale[1]);
	imu->data.magn.z = (float)((imu->raw.packed.magn.z - imu->cali.magn_offset[2]) * imu->cali.magn_scale[2]);
	
	return IMU_OK;
}
