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
	BMI_Read(BMI_ACCL, BMI088_ACCL_X_LSB_REG, gimu->raw, 6u);
	/* WTF. Why seperate these 2 reg? */
	//BMI_Read(BMI_ACCL, BMI088_TEMP_MSB_REG, gimu->raw.bytes + 6u, 2u);
	
}

void IMU_GyroIntCallback(void) {
	BMI_Read(BMI_ACCL, BMI088_GYRO_X_LSB_REG, gimu->raw, 6u);
}

/* Exported functions --------------------------------------------------------*/
int IMU_Init(IMU_t *imu) {
	if (imu == NULL)
		return IMU_ERR_NULL;
	
	if (inited)
		return IMU_ERR_INITED;
	
	/* Switch accl to SPI mode. */
	if (BMI_ReadSingle(BMI_ACCL, BMI088_ACCL_CHIP_ID_REG) != BMI088_ACCL_CHIP_ID)
		return IMU_ERR_NO_DEV;
	
	if (BMI_ReadSingle(BMI_GYRO, BMI088_GYRO_CHIP_ID_REG) != BMI088_GYRO_CHIP_ID)
		return IMU_ERR_NO_DEV;
	
	gimu = imu;
	inited = true;
	
	/* BMI088 init. */
	
	/* Accl init. */
	BMI_WriteSingle(BMI_ACCL, BMI088_ACCL_SOFTRESET_REG, 0xB6);
	BSP_Delay(3);
	
	/* Filter setting: Normal. Output data rate 400. */
	BMI_WriteSingle(BMI_ACCL, BMI088_ACCL_CONF_REG, 0xAA);
	
	/* 0x00: +-3G. 0x01: +-6G. 0x02: +-12G. 0x03: +-24G. */
	BMI_WriteSingle(BMI_ACCL, BMI088_ACCL_RANGE_REG, 0x01);
	
	/* INT1 as output. Open-dtrin. Active High. */
	BMI_WriteSingle(BMI_ACCL, BMI088_ACCL_INT1_IO_CONF_REG, 0x00);
	
	/* INT2 as output. Open-dtrin. Active High. */
	BMI_WriteSingle(BMI_ACCL, BMI088_ACCL_INT2_IO_CONF_REG, 0x00);
	
	/* Map data ready interrupt to INT1 and INT2. */
	BMI_WriteSingle(BMI_ACCL, BMI088_ACCL_INT1_INT2_MAP_DATA_REG, 0x44);
	
	/* Turn on accl. Now we can read data. */
	BMI_WriteSingle(BMI_ACCL, BMI088_ACCL_PWR_CTRL_REG, 0x44);
	BSP_Delay(50);
	
	/* Gyro init. */
	BMI_WriteSingle(BMI_GYRO, BMI088_GYRO_SOFTRESET_REG, 0xB6);
	BSP_Delay(3);
	
	/* 0x00: +-2000. 0x01: +-1000. 0x02: +-500. 0x03: +-250. 0x04: +-125. */
	BMI_WriteSingle(BMI_GYRO, BMI088_GYRO_RANGE_REG, 0x00);

	/* Filter bw: 47Hz. Output data rate 400. */
	BMI_WriteSingle(BMI_GYRO, BMI088_GYRO_RANGE_REG, 0x03);
	
	/* Map data ready interrupt to INT3 and INT4. */
	BMI_WriteSingle(BMI_GYRO, BMI088_GYRO_INT_CTRL_REG, 0x80);
	
	/* INT3 and INT4 as output. Push-pull. Active low. */
	BMI_WriteSingle(BMI_ACCL, BMI088_ACCL_INT2_IO_CONF_REG, 0x00);
	
	/* Map data ready interrupt to INT3 and INT3. */
	BMI_WriteSingle(BMI_ACCL, BMI088_GYRO_INT3_INT4_IO_MAP_REG, 0x81);
	
	BSP_SPI_RegisterCallback(BSP_SPI_IMU, BSP_SPI_RX_COMPLETE_CB, IMU_RxCpltCallback);
	BSP_GPIO_RegisterCallback(ACCL_INT_Pin, IMU_AcclIntCallback);
	BSP_GPIO_RegisterCallback(GYRO_INT_Pin, IMU_GyroIntCallback);
	
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
	return IMU_OK;
}

int IMU_ParseAccl(IMU_t *imu) {
	uint16_t temp_uint11 = (imu->raw[0] << 3) | (imu->raw[1] >> 5);
	
	if(temp_uint11 > 1023)
		temp_uint11 -= 2014;
	
	imu->temp = temp_uint11 * 0.125f + 21.f;
	
	imu->accl.x = (float)(imu->raw[1] | imu->raw[0]);
	imu->accl.y = (float)(imu->raw[3] | imu->raw[2]);
	imu->accl.z = (float)(imu->raw[5] | imu->raw[4]);
	
	return IMU_OK;
}

int IMU_ParseGyro(IMU_t *imu) {
	/* Gyroscope imu_raw -> degrees/sec -> radians/sec */
	float raw_x = (float)(imu->raw[1] | imu->raw[0]);
	float raw_y = (float)(imu->raw[3] | imu->raw[2]);
	float raw_z = (float)(imu->raw[5] | imu->raw[4]);
	
	imu->gyro.x = raw_x / 16.384f / 180.f * PI;
	imu->gyro.y = raw_y / 16.384f / 180.f * PI;
	imu->gyro.z = raw_z / 16.384f / 180.f * PI;
	
	return IMU_OK;
}
