#include "imu.h"
#include "board.h"
#include "main.h"

#include "spi.h"

#define MPU6500_SELF_TEST_XG        (0x00)
#define MPU6500_SELF_TEST_YG        (0x01)
#define MPU6500_SELF_TEST_ZG        (0x02)
#define MPU6500_SELF_TEST_XA        (0x0D)
#define MPU6500_SELF_TEST_YA        (0x0E)
#define MPU6500_SELF_TEST_ZA        (0x0F)
#define MPU6500_XG_OFFSET_H         (0x13)
#define MPU6500_XG_OFFSET_L         (0x14)
#define MPU6500_YG_OFFSET_H         (0x15)
#define MPU6500_YG_OFFSET_L         (0x16)
#define MPU6500_ZG_OFFSET_H         (0x17)
#define MPU6500_ZG_OFFSET_L         (0x18)
#define MPU6500_SMPLRT_DIV          (0x19)
#define MPU6500_CONFIG              (0x1A)
#define MPU6500_GYRO_CONFIG         (0x1B)
#define MPU6500_ACCEL_CONFIG        (0x1C)
#define MPU6500_ACCEL_CONFIG_2      (0x1D)
#define MPU6500_LP_ACCEL_ODR        (0x1E)
#define MPU6500_MOT_THR             (0x1F)
#define MPU6500_FIFO_EN             (0x23)
#define MPU6500_I2C_MST_CTRL        (0x24)
#define MPU6500_I2C_SLV0_ADDR       (0x25)
#define MPU6500_I2C_SLV0_REG        (0x26)
#define MPU6500_I2C_SLV0_CTRL       (0x27)
#define MPU6500_I2C_SLV1_ADDR       (0x28)
#define MPU6500_I2C_SLV1_REG        (0x29)
#define MPU6500_I2C_SLV1_CTRL       (0x2A)
#define MPU6500_I2C_SLV2_ADDR       (0x2B)
#define MPU6500_I2C_SLV2_REG        (0x2C)
#define MPU6500_I2C_SLV2_CTRL       (0x2D)
#define MPU6500_I2C_SLV3_ADDR       (0x2E)
#define MPU6500_I2C_SLV3_REG        (0x2F)
#define MPU6500_I2C_SLV3_CTRL       (0x30)
#define MPU6500_I2C_SLV4_ADDR       (0x31)
#define MPU6500_I2C_SLV4_REG        (0x32)
#define MPU6500_I2C_SLV4_DO         (0x33)
#define MPU6500_I2C_SLV4_CTRL       (0x34)
#define MPU6500_I2C_SLV4_DI         (0x35)
#define MPU6500_I2C_MST_STATUS      (0x36)
#define MPU6500_INT_PIN_CFG         (0x37)
#define MPU6500_INT_ENABLE          (0x38)
#define MPU6500_INT_STATUS          (0x3A)
#define MPU6500_ACCEL_XOUT_H        (0x3B)
#define MPU6500_ACCEL_XOUT_L        (0x3C)
#define MPU6500_ACCEL_YOUT_H        (0x3D)
#define MPU6500_ACCEL_YOUT_L        (0x3E)
#define MPU6500_ACCEL_ZOUT_H        (0x3F)
#define MPU6500_ACCEL_ZOUT_L        (0x40)
#define MPU6500_TEMP_OUT_H          (0x41)
#define MPU6500_TEMP_OUT_L          (0x42)
#define MPU6500_GYRO_XOUT_H         (0x43)
#define MPU6500_GYRO_XOUT_L         (0x44)
#define MPU6500_GYRO_YOUT_H         (0x45)
#define MPU6500_GYRO_YOUT_L         (0x46)
#define MPU6500_GYRO_ZOUT_H         (0x47)
#define MPU6500_GYRO_ZOUT_L         (0x48)
#define MPU6500_EXT_SENS_DATA_00    (0x49)
#define MPU6500_EXT_SENS_DATA_01    (0x4A)
#define MPU6500_EXT_SENS_DATA_02    (0x4B)
#define MPU6500_EXT_SENS_DATA_03    (0x4C)
#define MPU6500_EXT_SENS_DATA_04    (0x4D)
#define MPU6500_EXT_SENS_DATA_05    (0x4E)
#define MPU6500_EXT_SENS_DATA_06    (0x4F)
#define MPU6500_EXT_SENS_DATA_07    (0x50)
#define MPU6500_EXT_SENS_DATA_08    (0x51)
#define MPU6500_EXT_SENS_DATA_09    (0x52)
#define MPU6500_EXT_SENS_DATA_10    (0x53)
#define MPU6500_EXT_SENS_DATA_11    (0x54)
#define MPU6500_EXT_SENS_DATA_12    (0x55)
#define MPU6500_EXT_SENS_DATA_13    (0x56)
#define MPU6500_EXT_SENS_DATA_14    (0x57)
#define MPU6500_EXT_SENS_DATA_15    (0x58)
#define MPU6500_EXT_SENS_DATA_16    (0x59)
#define MPU6500_EXT_SENS_DATA_17    (0x5A)
#define MPU6500_EXT_SENS_DATA_18    (0x5B)
#define MPU6500_EXT_SENS_DATA_19    (0x5C)
#define MPU6500_EXT_SENS_DATA_20    (0x5D)
#define MPU6500_EXT_SENS_DATA_21    (0x5E)
#define MPU6500_EXT_SENS_DATA_22    (0x5F)
#define MPU6500_EXT_SENS_DATA_23    (0x60)
#define MPU6500_I2C_SLV0_DO         (0x63)
#define MPU6500_I2C_SLV1_DO         (0x64)
#define MPU6500_I2C_SLV2_DO         (0x65)
#define MPU6500_I2C_SLV3_DO         (0x66)
#define MPU6500_I2C_MST_DELAY_CTRL  (0x67)
#define MPU6500_SIGNAL_PATH_RESET   (0x68)
#define MPU6500_MOT_DETECT_CTRL     (0x69)
#define MPU6500_USER_CTRL           (0x6A)
#define MPU6500_PWR_MGMT_1          (0x6B)
#define MPU6500_PWR_MGMT_2          (0x6C)
#define MPU6500_FIFO_COUNTH         (0x72)
#define MPU6500_FIFO_COUNTL         (0x73)
#define MPU6500_FIFO_R_W            (0x74)
#define MPU6500_WHO_AM_I            (0x75)
#define MPU6500_XA_OFFSET_H         (0x77)
#define MPU6500_XA_OFFSET_L         (0x78)
#define MPU6500_YA_OFFSET_H         (0x7A)
#define MPU6500_YA_OFFSET_L         (0x7B)
#define MPU6500_ZA_OFFSET_H         (0x7D)
#define MPU6500_ZA_OFFSET_L         (0x7E)

#define MPU6500_ID									(0x70)

#define IST8310_WAI 								(0x00)
#define IST8310_STAT1 							(0x02)
#define IST8310_DATAXL 							(0x03)
#define IST8310_DATAXM 							(0x04)
#define IST8310_DATAYL 							(0x05)
#define IST8310_DATAYM 							(0x06)
#define IST8310_DATAZL 							(0x07)
#define IST8310_DATAZM 							(0x08)
#define IST8310_STAT2 							(0x08)
#define IST8310_CNTL1		 						(0x0A)
#define IST8310_CNTL2		 						(0x0B)
#define IST8310_STR 								(0x0C)
#define IST8310_TEMPL 							(0x1C)
#define IST8310_TEMPH 							(0x1D)
#define IST8310_AVGCNTL 						(0x41)
#define IST8310_PDCNTL 							(0x42)

#define IST8310_ADDRESS 						(0x0E)
#define IST8310_ID		 							(0x10)

#ifndef M_PI
#define M_PI 3.141592653589793238462643383f
#endif

#define NSS_Reset()	HAL_GPIO_WritePin(SPI5_NSS_GPIO_Port, SPI5_NSS_Pin, GPIO_PIN_RESET)
#define NSS_Set()	HAL_GPIO_WritePin(SPI5_NSS_GPIO_Port, SPI5_NSS_Pin, GPIO_PIN_SET)

#define IST_Reset()	HAL_GPIO_WritePin(CMPS_RST_GPIO_Port, CMPS_RST_Pin, GPIO_PIN_RESET)
#define IST_Set()	HAL_GPIO_WritePin(CMPS_RST_GPIO_Port, CMPS_RST_Pin, GPIO_PIN_SET)

static uint8_t tx, rx;
static uint8_t buffer[20];

extern SPI_HandleTypeDef hspi5;

static __packed struct {
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
}raw;

static int16_t gyro_offset[3] = {0};
static int16_t magn_offset[3] = {0};
static int16_t magn_scale[3] = {0};

/* Do NOT use hardware NSS. It doesn't implement the same logic. */
static void IMU_MpuWrite(const uint8_t reg, uint8_t data) {
	tx = (reg  & 0x7f);
	
	NSS_Reset();
	
	HAL_SPI_Transmit(&hspi5, &tx, 1, 55);
	HAL_SPI_Transmit(&hspi5, &data, 1, 55);
	
	NSS_Set();
	
	Board_Delay(10);
}

static void IMU_MpuRead(const uint8_t reg, uint8_t* p_data, uint8_t len) {
	if (p_data == NULL)
		return;
	
	tx = (reg | 0x80);
	
	NSS_Reset();;
	
	HAL_SPI_Transmit(&hspi5, &tx, 1, 55);
	HAL_SPI_Receive(&hspi5, p_data, len, 55);
	
	NSS_Set();
}

static void IMU_IstWrite(const uint8_t reg, uint8_t const data) {
	IMU_MpuWrite(MPU6500_I2C_SLV4_ADDR, IST8310_ADDRESS);
	IMU_MpuWrite(MPU6500_I2C_SLV4_REG, reg);
	IMU_MpuWrite(MPU6500_I2C_SLV4_DO, data);
	IMU_MpuWrite(MPU6500_I2C_SLV4_CTRL, 0x80);
}

/* Make sure the RESET pin of ist8310 is setted to HIGH or FLOATing. */
static void IMU_IstRead(const uint8_t reg, uint8_t* p_data) {
	if (p_data == NULL)
		return;
	
	IMU_MpuWrite(MPU6500_I2C_SLV4_ADDR, 0x80 | IST8310_ADDRESS);
	IMU_MpuWrite(MPU6500_I2C_SLV4_REG, reg);
	IMU_MpuWrite(MPU6500_I2C_SLV4_CTRL, 0x80);
	IMU_MpuRead(MPU6500_I2C_SLV4_DI, p_data, 1);
}

/* Remove gyro static error. Be careful of overflow. */
static void IMU_CaliGyro(void) {
	
	for(uint8_t i = 0; i < 100; i++) {
		IMU_MpuRead(MPU6500_GYRO_XOUT_H, buffer, 6);
		
		for(uint8_t i = 0; i < 6; i += 2)
			gyro_offset[i/2] += (buffer[i] << 8) | buffer[i+1];
		
		Board_Delay(5);
	}
	
	for(uint8_t i = 0; i < 3; i++)
		gyro_offset[i] /= 100;
}

Board_Status_t IMU_Init(void) {
	IST_Reset();
	Board_Delay(5);
	IST_Set();
	Board_Delay(5);
	
	IMU_MpuRead(MPU6500_WHO_AM_I, &rx, 1);
	if (rx != MPU6500_ID)
		return BOARD_FAIL;
	
	/* MPU6500 init. */
	IMU_MpuWrite(MPU6500_PWR_MGMT_1, 0x80); /* Reset device */
	Board_Delay(100);
	IMU_MpuWrite(MPU6500_SIGNAL_PATH_RESET, 0x0f); /* Reset device */
	Board_Delay(100);
	
	IMU_MpuWrite(MPU6500_PWR_MGMT_1, 0x03); /* Clock source -> gyro-z */
	IMU_MpuWrite(MPU6500_PWR_MGMT_2, 0x00); /* Enable acc & gyro */
	IMU_MpuWrite(MPU6500_CONFIG, 0x04); /* LPF 41Hz */
	IMU_MpuWrite(MPU6500_GYRO_CONFIG, 0x18); /* gyro range 0x10:+-1000dps 0x18:+-2000dps */
	IMU_MpuWrite(MPU6500_ACCEL_CONFIG, 0x18); /* +-2G */
	IMU_MpuWrite(MPU6500_ACCEL_CONFIG_2, 0x02); /* Enable and set acc LPF */
	
	/* IST8310 init. */
	IMU_MpuWrite(MPU6500_USER_CTRL, 0x30); /* Enable IIC master mode. */
	IMU_MpuWrite(MPU6500_I2C_MST_CTRL, 0x0d); /* Set IIC 400kHz. */
	
	Board_Delay(100);
	
	IMU_IstRead(IST8310_WAI, &rx);
	if (rx != IST8310_ID)
		return BOARD_FAIL;
	
	IMU_IstWrite(IST8310_CNTL1, 0x00); /* Config as ready mode to access register */
	IMU_IstWrite(IST8310_CNTL2, 0x00); /* Disable interupt */
	IMU_IstWrite(IST8310_AVGCNTL, 0x12); /* Set low noise mode. */
	IMU_IstWrite(IST8310_PDCNTL, 0xc0); /* Set pulse duration normal mode. */
	 
	/* Setup auto read. */

	/* Set slv0 to auto write. */
	IMU_MpuWrite(MPU6500_I2C_SLV0_ADDR, IST8310_ADDRESS);
	IMU_MpuWrite(MPU6500_I2C_SLV0_REG, IST8310_CNTL1);
	IMU_MpuWrite(MPU6500_I2C_SLV0_DO, 0x01);
	
	/* Set slv1 to auto read. */
	IMU_MpuWrite(MPU6500_I2C_SLV1_ADDR, (0x80 | IST8310_ADDRESS));
	IMU_MpuWrite(MPU6500_I2C_SLV1_REG, IST8310_DATAXL);
	
	/* Enable slv0 and slv1 access delay */
	IMU_MpuWrite(MPU6500_I2C_MST_DELAY_CTRL, 0x03);
	
	/* Enable slv0 auto transmit */
	IMU_MpuWrite(MPU6500_I2C_SLV0_CTRL, 0x81);
  
	/* Wait 6ms (minimum waiting time for 16 times internal average setup) */
	/* enable slave 0 with data_num bytes reading */
	IMU_MpuWrite(MPU6500_I2C_SLV1_CTRL, 0xd6);
	
	IMU_CaliGyro();
	
	return BOARD_OK;
}

/* magn_scale[3] is initially zero. So data from uncalibrated magnentmeter is ignored. */
/*         x
 *         ^
 *     y < R （RoboMaster trademark）
 *     UP is z
 */

Board_Status_t IMU_Update(IMU_t* himu) {
	if (himu == NULL)
		return BOARD_FAIL;
	
	IMU_MpuRead(MPU6500_ACCEL_XOUT_H, buffer, 20);
	
	/* View "struct raw" as "array raw", which need "raw" to be a packed struct. */
	for(uint8_t i = 0; i < 20; i += 2)
		((int16_t*) & raw)[i/2] = (buffer[i] << 8) | buffer[i+1];
	
	himu->data.temp = 21.f + (float)raw.temp / 333.87f;
	
	/* Rotation and remap are added to all sensor. */
	
	himu->data.accl.x = -(float)raw.accl.y;
	himu->data.accl.y = (float)raw.accl.x;
	himu->data.accl.z = (float)raw.accl.z;
	
	/* Convert gyroscope raw to degrees/sec, then, to radians/sec */
	himu->data.gyro.x = -(float)(raw.gyro.y - gyro_offset[1]) / 16.384f / 180.f * M_PI;
	himu->data.gyro.y = (float)(raw.gyro.x - gyro_offset[0]) / 16.384f / 180.f * M_PI;
	himu->data.gyro.z = (float)(raw.gyro.z - gyro_offset[2]) / 16.384f / 180.f * M_PI;
	
	himu->data.magn.x = (float) ((raw.magn.x - magn_offset[0]) * magn_scale[0]);
	himu->data.magn.y = (float) ((raw.magn.y - magn_offset[1]) * magn_scale[1]);
	himu->data.magn.z = -(float) ((raw.magn.z - magn_offset[2]) * magn_scale[2]);
	
	return BOARD_OK;
}
