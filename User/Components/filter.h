/* 
	二阶低通滤波器。

*/

#pragma once

#include "user_math.h"

typedef struct {
	float32_t cutoff_freq;
	
	float32_t a1;
	float32_t a2;

	float32_t b0;
	float32_t b1;
	float32_t b2;
	
	float32_t delay_element_1;
	float32_t delay_element_2;
		
} LowPassFilter2p_t;

typedef struct {
	float32_t notch_freq;
	float32_t bandwidth;
	
	float32_t a1;
	float32_t a2;

	float32_t b0;
	float32_t b1;
	float32_t b2;
	float32_t delay_element_1;
	float32_t delay_element_2;
		
} NotchFilter_t;

void LowPassFilter2p_Init(LowPassFilter2p_t *f, float32_t sample_freq, float32_t cutoff_freq);
float32_t LowPassFilter2p_Apply(LowPassFilter2p_t *f, float32_t sample);
float32_t LowPassFilter2p_Reset(LowPassFilter2p_t *f, float32_t sample);

void NotchFilter_Init(NotchFilter_t *f, float32_t sample_freq, float32_t notch_freq, float32_t bandwidth);
float32_t NotchFilter_Apply(NotchFilter_t *f, float32_t sample);
float32_t NotchFilter_Reset(NotchFilter_t *f, float32_t sample);
