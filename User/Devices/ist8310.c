/* 
	IST8310 地磁传感器。

*/

/* Includes ------------------------------------------------------------------*/
#include "ist8310.h"

/* Include 标准库。*/
#include "stdbool.h"
#include "string.h"

/* Include BSP相关的头文件 */
#include "bsp_delay.h"
#include "bsp_i2c.h"
#include "bsp_gpio.h"
#include "gpio.h"

/* Include Component相关的头文件 */
/* Private define ------------------------------------------------------------*/
#define IST8310_WAI 			(0x00)
#define IST8310_STAT1	(0x02)
#define IST8310_DATAXL	(0x03)
#define IST8310_STAT2	(0x09)
#define IST8310_CNTL1	(0x0A)
#define IST8310_CNTL2	(0x0B)
#define IST8310_STR		(0x0C)
#define IST8310_TEMPL	(0x1C)
#define IST8310_TEMPH	(0x1D)
#define IST8310_AVGCNTL	(0x41)
#define IST8310_PDCNTL	(0x42)

#define IST8310_CHIP_ID				(0x10)

#define IST8310_IIC_ADDRESS			(0x0E)

/* Private macro -------------------------------------------------------------*/
#define IST8310_GYRO_NSS_SET()		HAL_GPIO_WritePin(CMPS_RST_GPIO_Port, CMPS_RST_Pin, GPIO_PIN_SET)
#define IST8310_GYRO_NSS_RESET()	HAL_GPIO_WritePin(CMPS_RST_GPIO_Port, CMPS_RST_Pin, GPIO_PIN_RESET)

/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static IST8310_t *gist8310 = NULL;
static bool inited = false;

/* Private function  ---------------------------------------------------------*/
static void IST8310_WriteSingle(uint8_t reg, uint8_t data) {
	BSP_I2C_Transmit(BSP_I2C_COMP, reg, &data, 2u, 20u);
	
}

static uint8_t IST8310_ReadSingle(uint8_t reg) {
	uint8_t buffer;
	BSP_I2C_Receive(BSP_I2C_COMP, reg, &buffer, 2u, 20u);
	
	return buffer;
}

static void IST8310_Read(uint8_t reg, uint8_t *data, uint8_t len) {
	if (data == NULL)
		return;
	
	BSP_I2C_ReceiveDMA(BSP_I2C_COMP, reg, data, len);
}

static void IST8310_MasterRxCpltCallback(void) {
	osThreadFlagsSet(gist8310->received_alert, IST8310_SIGNAL_MAGN_RAW_REDY);
}

static void IST8310_IntCallback(void) {
	osThreadFlagsSet(gist8310->received_alert, IST8310_SIGNAL_MAGN_NEW_DATA);
}

/* Exported functions --------------------------------------------------------*/
int IST8310_Init(IST8310_t *ist8310) {
	if (ist8310 == NULL)
		return IST8310_ERR_NULL;
	
	if (inited)
		return IST8310_ERR_INITED;
	
	IST8310_WriteSingle(IST8310_CNTL2, 0x01);
	BSP_Delay(50);
	
	if (IST8310_ReadSingle(IST8310_WAI) != IST8310_CHIP_ID)
		return IST8310_ERR_NO_DEV;
	
	BSP_GPIO_DisableIRQ(ACCL_INT_Pin);
	BSP_I2C_RegisterCallback(BSP_I2C_COMP, HAL_I2C_MASTER_RX_COMPLETE_CB, IST8310_MasterRxCpltCallback);
	BSP_GPIO_RegisterCallback(ACCL_INT_Pin, IST8310_IntCallback);
	
	/* Init. */
	IST8310_WriteSingle(IST8310_CNTL2, 0x08);
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

int IST8310_Receive(IST8310_t *ist8310){
	IST8310_Read(IST8310_DATAXL, gist8310->raw, 6u);
	return IST8310_OK;
}

int IST8310_Parse(IST8310_t *ist8310) {
	if (ist8310 == NULL)
		return IST8310_ERR_NULL;

	const int16_t raw_x = ((ist8310->raw[1] << 8) | ist8310->raw[0]);
	const int16_t raw_y = ((ist8310->raw[3] << 8) | ist8310->raw[2]);
	const int16_t raw_z = ((ist8310->raw[5] << 8) | ist8310->raw[4]);
	
	ist8310->magn.x = (float)raw_x / 3.3f;
	ist8310->magn.y = (float)raw_y / 3.3f;
	ist8310->magn.z = (float)raw_z / 3.3f;
	
	return IST8310_OK;
}

