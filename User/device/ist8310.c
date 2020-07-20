/* 
	IST8310 地磁传感器。

*/

/* Includes ------------------------------------------------------------------*/
#include "ist8310.h"

#include <stdbool.h>
#include <string.h>

#include "gpio.h"

#include "bsp\delay.h"
#include "bsp\i2c.h"
#include "bsp\gpio.h"

/* Private define ------------------------------------------------------------*/
#define IST8310_WAI				(0x00)
#define IST8310_STAT1			(0x02)
#define IST8310_DATAXL			(0x03)
#define IST8310_STAT2			(0x09)
#define IST8310_CNTL1			(0x0A)
#define IST8310_CNTL2			(0x0B)
#define IST8310_STR				(0x0C)
#define IST8310_TEMPL			(0x1C)
#define IST8310_TEMPH			(0x1D)
#define IST8310_AVGCNTL			(0x41)
#define IST8310_PDCNTL			(0x42)

#define IST8310_CHIP_ID			(0x10)

#define IST8310_IIC_ADDRESS		(0x0E << 1)

#define IST8310_LEN_RX_BUFF (6)
/* Private macro -------------------------------------------------------------*/
#define IST8310_GYRO_NSS_SET()		HAL_GPIO_WritePin(CMPS_RST_GPIO_Port, CMPS_RST_Pin, GPIO_PIN_SET)
#define IST8310_GYRO_NSS_RESET()	HAL_GPIO_WritePin(CMPS_RST_GPIO_Port, CMPS_RST_Pin, GPIO_PIN_RESET)

/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t ist8310_rxbuf[IST8310_LEN_RX_BUFF];

static IST8310_t *gist8310;
static bool inited = false;

/* Private function  ---------------------------------------------------------*/
static void IST8310_WriteSingle(uint8_t reg, uint8_t data) {
	HAL_I2C_Mem_Write(BSP_I2C_GetHandle(BSP_I2C_COMP), IST8310_IIC_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

static uint8_t IST8310_ReadSingle(uint8_t reg) {
	uint8_t buf = 0;
	HAL_I2C_Mem_Read(BSP_I2C_GetHandle(BSP_I2C_COMP), IST8310_IIC_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, &buf, 1, 100);
	return buf;
}

static void IST8310_Read(uint8_t reg, uint8_t *data, uint8_t len) {
	if (data == NULL)
		return;
	
	HAL_I2C_Mem_Read_DMA(BSP_I2C_GetHandle(BSP_I2C_COMP), IST8310_IIC_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, data, len);
}

static void IST8310_MemRxCpltCallback(void) {
	osThreadFlagsSet(gist8310->thread_alert, IST8310_SIGNAL_MAGN_RAW_REDY);
}

static void IST8310_IntCallback(void) {
	IST8310_Read(IST8310_DATAXL, ist8310_rxbuf, IST8310_LEN_RX_BUFF);
}

/* Exported functions --------------------------------------------------------*/
int8_t IST8310_Init(IST8310_t *ist8310, osThreadId_t thread_alert) {
	if (ist8310 == NULL)
		return IST8310_ERR_NULL;
	
	if (inited)
		return IST8310_ERR_INITED;
	
	ist8310->thread_alert = thread_alert;
	
	IST8310_GYRO_NSS_RESET();
	BSP_Delay(50);
	IST8310_GYRO_NSS_SET();
	BSP_Delay(50);
	
	if (IST8310_ReadSingle(IST8310_WAI) != IST8310_CHIP_ID)
		return IST8310_ERR_NO_DEV;
	
	BSP_GPIO_DisableIRQ(ACCL_INT_Pin);
	
	BSP_I2C_RegisterCallback(BSP_I2C_COMP, HAL_I2C_MEM_RX_CPLT_CB, IST8310_MemRxCpltCallback);
	BSP_GPIO_RegisterCallback(ACCL_INT_Pin, IST8310_IntCallback);
	
	/* Init. */
	/* 0x00: Stand-By mode. 0x01: Single measurement mode. */
	
	/* 0x08: Data ready function enable. 0x04: DRDY signal active */
	IST8310_WriteSingle(IST8310_CNTL2, 0x0C);
	
	IST8310_WriteSingle(IST8310_AVGCNTL, 0x02);
	IST8310_WriteSingle(IST8310_PDCNTL, 0xC0);
	BSP_Delay(10);
	
	gist8310 = ist8310;
	inited = true;
	
	BSP_GPIO_EnableIRQ(ACCL_INT_Pin);
	return IST8310_OK;
}

IST8310_t *IST8310_GetDevice(void) {
	if (inited) {
		return gist8310;
	}
	return NULL;
}

int8_t IST8310_Receive(IST8310_t *ist8310) {
	IST8310_WriteSingle(IST8310_CNTL1, 0x01);
	return IST8310_OK;
}

int8_t IST8310_Parse(IST8310_t *ist8310) {
	if (ist8310 == NULL)
		return IST8310_ERR_NULL;

	const int16_t *raw_x = (int16_t *)(ist8310_rxbuf + 0);
	const int16_t *raw_y = (int16_t *)(ist8310_rxbuf + 2);
	const int16_t *raw_z = (int16_t *)(ist8310_rxbuf + 4);
	
	ist8310->magn.x = (float32_t)*raw_x * 3.f / 20.f;
	ist8310->magn.y = (float32_t)*raw_y * 3.f / 20.f;
	ist8310->magn.z = (float32_t)*raw_z * 3.f / 20.f;
	
	return IST8310_OK;
}
