/* 
	IST8310 地磁传感器。

*/

/* Includes ------------------------------------------------------------------*/
/* Include 自身的头文件。*/
#include "compass.h"

/* Include 标准库。*/
/* Include HAL相关的头文件。*/
//#include "i2c.h"

/* Include Component相关的头文件 */
/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

/* magn_scale[3] is initially zero. So data from uncalibrated magnentmeter is ignored. */
int COMP_Parse(COMP_t *comp) {
	if (comp == NULL)
		return COMP_ERR_NULL;

	comp->magn.x = (float)((comp->raw[0] - comp->cali.magn_offset[0]) * comp->cali.magn_scale[0]);
	comp->magn.y = (float)((comp->raw[0] - comp->cali.magn_offset[1]) * comp->cali.magn_scale[1]);
	comp->magn.z = (float)((comp->raw[0] - comp->cali.magn_offset[2]) * comp->cali.magn_scale[2]);
	
	return COMP_OK;
}

