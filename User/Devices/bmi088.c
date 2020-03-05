/* 
	BMI088 陀螺仪+加速度计传感器。

*/

/* Includes ------------------------------------------------------------------*/
#include "bmi088.h"

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
#define BMI088_ACCL_CHIP_ID_REG				(0x00)
#define BMI088_ACCL_ERR_REG					(0x02)
#define BMI088_ACCL_STATUS_REG				(0x03)
#define BMI088_ACCL_X_LSB_REG				(0x12)
#define BMI088_ACCL_X_MSB_REG				(0x13)
#define BMI088_ACCL_Y_LSB_REG				(0x14)
#define BMI088_ACCL_Y_MSB_REG				(0x15)
#define BMI088_ACCL_Z_LSB_REG				(0x16)
#define BMI088_ACCL_Z_MSB_REG				(0x17)
#define BMI088_ACCL_SENSORTIME_0_REG		(0x18)
#define BMI088_ACCL_SENSORTIME_1_REG		(0x19)
#define BMI088_ACCL_SENSORTIME_2_REG		(0x1A)
#define BMI088_ACCL_INT_STAT_1_REG			(0x1D)
#define BMI088_TEMP_MSB_REG					(0x22)
#define BMI088_TEMP_LSB_REG					(0x23)
#define BMI088_ACCL_CONF_REG				(0x40)
#define BMI088_ACCL_RANGE_REG				(0x41)
#define BMI088_ACCL_INT1_IO_CONF_REG		(0x53)
#define BMI088_ACCL_INT2_IO_CONF_REG		(0x54)
#define BMI088_ACCL_INT1_INT2_MAP_DATA_REG	(0x58)
#define BMI088_ACCL_SELF_TEST_REG			(0x6D)
#define BMI088_ACCL_PWR_CONF_REG			(0x7C)
#define BMI088_ACCL_PWR_CTRL_REG			(0x7D)
#define BMI088_ACCL_SOFTRESET_REG			(0x7E)

#define BMI088_ACCL_CHIP_ID					(0x1E)

#define BMI088_GYRO_CHIP_ID_REG				(0x00)
#define BMI088_GYRO_X_LSB_REG				(0x02)
#define BMI088_GYRO_X_MSB_REG				(0x03)
#define BMI088_GYRO_Y_LSB_REG				(0x04)
#define BMI088_GYRO_Y_MSB_REG				(0x05)
#define BMI088_GYRO_Z_LSB_REG				(0x06)
#define BMI088_GYRO_Z_MSB_REG				(0x07)
#define BMI088_GYRO_INT_STAT_1_REG			(0x0A)
#define BMI088_GYRO_RANGE_REG				(0x0F)
#define BMI088_GYRO_BANDWIDTH_REG			(0x10)
#define BMI088_GYRO_LPM1_REG				(0x11)
#define BMI088_GYRO_SOFTRESET_REG			(0x14)
#define BMI088_GYRO_INT_CTRL_REG			(0x15)
#define BMI088_GYRO_INT3_INT4_IO_CONF_REG	(0x16)
#define BMI088_GYRO_INT3_INT4_IO_MAP_REG	(0x18)
#define BMI088_GYRO_SELF_TEST_REG			(0x3C)

#define BMI088_GYRO_CHIP_ID					(0x0F)

/* Private macro -------------------------------------------------------------*/
#define BMI088_ACCL_NSS_SET()		HAL_GPIO_WritePin(ACCL_CS_GPIO_Port, ACCL_CS_Pin, GPIO_PIN_SET)
#define BMI088_ACCL_NSS_RESET()	HAL_GPIO_WritePin(ACCL_CS_GPIO_Port, ACCL_CS_Pin, GPIO_PIN_RESET)

#define BMI088_GYRO_NSS_SET()		HAL_GPIO_WritePin(GYRO_CS_GPIO_Port, GYRO_CS_Pin, GPIO_PIN_SET)
#define BMI088_GYRO_NSS_RESET()	HAL_GPIO_WritePin(GYRO_CS_GPIO_Port, GYRO_CS_Pin, GPIO_PIN_RESET)
		
/* Private typedef -----------------------------------------------------------*/
typedef enum {
	BMI_ACCL,
	BMI_GYRO,
} BMI_Device_t;

/* Private variables ---------------------------------------------------------*/
static uint8_t buffer[2];
static BMI088_t *gimu;
static bool inited = false;

/* Private function  ---------------------------------------------------------*/
static void BMI_WriteSingle(BMI_Device_t dv, uint8_t reg, uint8_t data) {
	buffer[0] = (reg  & 0x7f);
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
	
	BSP_SPI_Transmit(BSP_SPI_IMU, buffer, 2u, 20u);
	
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
	buffer[0] = (reg | 0x80);
	BSP_SPI_Transmit(BSP_SPI_IMU, buffer, 1u, 20u);
	BSP_SPI_Receive(BSP_SPI_IMU, buffer, 2u, 20u);
	
	switch (dv) {
		case BMI_ACCL:
			BMI088_ACCL_NSS_SET();
			return buffer[1];
		
		case BMI_GYRO:
			BMI088_GYRO_NSS_SET();
			return buffer[0];
		
		default:
			return 0;
	}
}

static void BMI_Read(BMI_Device_t dv, uint8_t reg, uint8_t *data, uint8_t len) {
	if (data == NULL)
		return;
	
	switch (dv) {
		case BMI_ACCL:
			BMI088_ACCL_NSS_RESET();
			break;
		
		case BMI_GYRO:
			BMI088_GYRO_NSS_RESET();
			break;
	}
	buffer[0] = (reg | 0x80);
	BSP_SPI_Transmit(BSP_SPI_IMU, buffer, 1u, 20u);
	BSP_SPI_ReceiveDMA(BSP_SPI_IMU, data, len);
}

static void BMI088_RxCpltCallback(void) {
	if (HAL_GPIO_ReadPin(ACCL_CS_GPIO_Port, ACCL_CS_Pin) == GPIO_PIN_RESET) {
		BMI088_ACCL_NSS_SET();
		osThreadFlagsSet(gimu->received_alert, BMI088_SIGNAL_ACCL_RAW_REDY);
	}
	if (HAL_GPIO_ReadPin(GYRO_CS_GPIO_Port, GYRO_CS_Pin) == GPIO_PIN_RESET) {
		BMI088_GYRO_NSS_SET();
		osThreadFlagsSet(gimu->received_alert, BMI088_SIGNAL_GYRO_RAW_REDY);
	}
}

static void BMI088_AcclIntCallback(void) {
	osThreadFlagsSet(gimu->received_alert, BMI088_SIGNAL_ACCL_NEW_DATA);
}

static void BMI088_GyroIntCallback(void) {
	osThreadFlagsSet(gimu->received_alert, BMI088_SIGNAL_GYRO_NEW_DATA);
}

/* Exported functions --------------------------------------------------------*/
int BMI088_Init(BMI088_t *bmi088) {
	if (bmi088 == NULL)
		return BMI088_ERR_NULL;
	
	if (inited)
		return BMI088_ERR_INITED;
	
	BMI_WriteSingle(BMI_ACCL, BMI088_ACCL_SOFTRESET_REG, 0xB6);
	BMI_WriteSingle(BMI_GYRO, BMI088_GYRO_SOFTRESET_REG, 0xB6);
	BSP_Delay(30);
	
	/* Switch accl to SPI mode. */
	BMI_ReadSingle(BMI_ACCL, BMI088_ACCL_CHIP_ID_REG);
	
	if (BMI_ReadSingle(BMI_ACCL, BMI088_ACCL_CHIP_ID_REG) != BMI088_ACCL_CHIP_ID)
		return BMI088_ERR_NO_DEV;
	
	if (BMI_ReadSingle(BMI_GYRO, BMI088_GYRO_CHIP_ID_REG) != BMI088_GYRO_CHIP_ID)
		return BMI088_ERR_NO_DEV;
	
	
	BSP_GPIO_DisableIRQ(ACCL_INT_Pin);
	BSP_GPIO_DisableIRQ(GYRO_INT_Pin);
	
	BSP_SPI_RegisterCallback(BSP_SPI_IMU, BSP_SPI_RX_COMPLETE_CB, BMI088_RxCpltCallback);
	BSP_GPIO_RegisterCallback(ACCL_INT_Pin, BMI088_AcclIntCallback);
	BSP_GPIO_RegisterCallback(GYRO_INT_Pin, BMI088_GyroIntCallback);
	
	/* Accl init. */
	/* Filter setting: Normal. */
	/* ODR: 0xAA: 400Hz. 0xA9: 200Hz. 0xA8: 100Hz. 0xA6: 25Hz. */
	BMI_WriteSingle(BMI_ACCL, BMI088_ACCL_CONF_REG, 0xA8);
	
	/* 0x00: +-3G. 0x01: +-6G. 0x02: +-12G. 0x03: +-24G. */
	BMI_WriteSingle(BMI_ACCL, BMI088_ACCL_RANGE_REG, 0x00);
	
	/* INT1 as output. Push-pull. Active low. Output. */
	BMI_WriteSingle(BMI_ACCL, BMI088_ACCL_INT1_IO_CONF_REG, 0x08);
	
	/* Map data ready interrupt to INT1. */
	BMI_WriteSingle(BMI_ACCL, BMI088_ACCL_INT1_INT2_MAP_DATA_REG, 0x04);
	
	/* Turn on accl. Now we can read data. */
	BMI_WriteSingle(BMI_ACCL, BMI088_ACCL_PWR_CTRL_REG, 0x04);
	BSP_Delay(50);
	
	/* Gyro init. */
	/* 0x00: +-2000. 0x01: +-1000. 0x02: +-500. 0x03: +-250. 0x04: +-125. */
	BMI_WriteSingle(BMI_GYRO, BMI088_GYRO_RANGE_REG, 0x01);

	/* Filter bw: 47Hz. */
	/* ODR: 0x03: 400Hz. 0x06: 200Hz. 0x07: 100Hz. */
	BMI_WriteSingle(BMI_GYRO, BMI088_GYRO_BANDWIDTH_REG, 0x07);
	
	/* INT3 and INT4 as output. Push-pull. Active low. */
	BMI_WriteSingle(BMI_GYRO, BMI088_GYRO_INT3_INT4_IO_CONF_REG, 0x00);
	
	/* Map data ready interrupt to INT3. */
	BMI_WriteSingle(BMI_GYRO, BMI088_GYRO_INT3_INT4_IO_MAP_REG, 0x01);
	
	/* Enable new data interrupt. */
	BMI_WriteSingle(BMI_GYRO, BMI088_GYRO_INT_CTRL_REG, 0x80);
	
	BSP_Delay(10);
	
	gimu = bmi088;
	inited = true;
	
	BSP_GPIO_EnableIRQ(ACCL_INT_Pin);
	BSP_GPIO_EnableIRQ(GYRO_INT_Pin);
	return BMI088_OK;
}

BMI088_t *BMI088_GetDevice(void) {
	if (inited) {
		return gimu;
	}
	return NULL;
}

int BMI088_ReceiveAccl(BMI088_t *bmi088) {
	BMI_Read(BMI_ACCL, BMI088_ACCL_X_LSB_REG, gimu->raw, 7u);
	return BMI088_OK;
	//BMI_Read(BMI_ACCL, BMI088_TEMP_MSB_REG, gimu->raw + 6u, 2u);
}
int BMI088_ReceiveGyro(BMI088_t *bmi088) {
	BMI_Read(BMI_GYRO, BMI088_GYRO_X_LSB_REG, &gimu->raw[7], 6u);
	return BMI088_OK;
}

int BMI088_ParseAccl(BMI088_t *bmi088) {
	uint16_t raw_temp = (bmi088->raw[0] << 3) | (bmi088->raw[1] >> 5);
	
	if(raw_temp > 1023)
		raw_temp -= 2014;
	
	bmi088->temp = raw_temp * 0.125f + 21.f;
	
	const int16_t raw_x = ((bmi088->raw[2] << 8) | bmi088->raw[1]);
	const int16_t raw_y = ((bmi088->raw[4] << 8) | bmi088->raw[3]);
	const int16_t raw_z = ((bmi088->raw[6] << 8) | bmi088->raw[5]);
	
	/* 3G: 10920. 6G: 5460. 12G: 2730. 24G: 1365. */
	bmi088->accl.x = (float)raw_x / 10920.f;
	bmi088->accl.y = (float)raw_y / 10920.f;
	bmi088->accl.z = (float)raw_z / 10920.f;
	
	return BMI088_OK;
}

int BMI088_ParseGyro(BMI088_t *bmi088) {
	/* Gyroscope imu_raw -> degrees/sec -> radians/sec */
	const int16_t raw_x = ((bmi088->raw[8] << 8) | bmi088->raw[7]);
	const int16_t raw_y = ((bmi088->raw[10] << 8) | bmi088->raw[9]);
	const int16_t raw_z = ((bmi088->raw[12] << 8) | bmi088->raw[11]);
	
	/* FS125: 262.144. FS250: 131.072. FS500: 65.536. FS1000: 32.768. FS2000: 16.384.*/
	bmi088->gyro.x = raw_x / 32.768f * MATH_DEGREE_TO_RADIAN_MULTIPLIER;
	bmi088->gyro.y = raw_y / 32.768f * MATH_DEGREE_TO_RADIAN_MULTIPLIER;
	bmi088->gyro.z = raw_z / 32.768f * MATH_DEGREE_TO_RADIAN_MULTIPLIER;
	
	return BMI088_OK;
}

float BMI088_GetUpdateFreq(BMI088_t *bmi088) {
	return 100.f;
}
