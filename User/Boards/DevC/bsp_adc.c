/* Includes ------------------------------------------------------------------*/
#include "bsp_adc.h"
#include "adc.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
volatile float voltage_vrefint_proportion = 8.0586080586080586080586080586081e-4f;

/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
float BSP_GetTemprater(void) {
	ADC_ChannelConfTypeDef sConfig;
	
	sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	sConfig.Offset = 0;
	
  
	if(HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
		Error_Handler();
	
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 5u);
	uint32_t raw = HAL_ADC_GetValue(&hadc1);
	float temp = (float)raw * voltage_vrefint_proportion;
	return (temp - 0.76f) * 400.0f + 25.0f;
}

float BSP_GetBatteryVoltage(void) {
	ADC_ChannelConfTypeDef sConfig;
	
	sConfig.Channel = ADC_CHANNEL_8;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	sConfig.Offset = 0;
	
  
	if(HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
		Error_Handler();
	
	HAL_ADC_Start(&hadc3);
	HAL_ADC_PollForConversion(&hadc3, 5u);
	uint32_t raw = HAL_ADC_GetValue(&hadc3);
	float voltage = (float)raw * voltage_vrefint_proportion * 10.090909090909090909090909090909f;

    return voltage;
}
uint8_t BSP_GetHardwareVersion(void) {
	return 1u;
}
