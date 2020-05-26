/* 
	带阻滤波器。

*/

#include "notch_filter.h"

#include "user_math.h"



void NotchFilter_Init(NotchFilter_t *f, float sample_freq, float notch_freq, float bandwidth) {
	f->notch_freq = notch_freq;
	f->bandwidth = bandwidth;
	
	f->delay_element_1 = 0.0f;
	f->delay_element_2 = 0.0f;
	
	if (notch_freq <= 0.f) {
		// no filtering
		f->b0 = 1.0f;
		f->b1 = 0.0f;
		f->b2 = 0.0f;

		f->a1 = 0.0f;
		f->a2 = 0.0f;

		return;
	}

	const float alpha = tanf(PI * bandwidth / sample_freq);
	const float beta = -cosf(2.f * PI* notch_freq / sample_freq);
	const float a0_inv = 1.f / (alpha + 1.f);

	f->b0 = a0_inv;
	f->b1 = 2.f * beta * a0_inv;
	f->b2 = a0_inv;

	f->a1 = f->b1;
	f->a2 = (1.f - alpha) * a0_inv;
}

inline float NotchFilter_Apply(NotchFilter_t *f, float sample) {
	// Direct Form II implementation
	const float delay_element_0 = sample - f->delay_element_1 * f->a1 - f->delay_element_2 * f->a2;
	const float output = delay_element_0 * f->b0 + f->delay_element_1 * f->b1 + f->delay_element_2 * f->b2;

	f->delay_element_2 = f->delay_element_1;
	f->delay_element_1 = delay_element_0;

	return output;
}

float NotchFilter_Reset(NotchFilter_t *f, float sample) {
	float dval = sample;

	if (fabsf(f->b0 + f->b1 + f->b2) > FLT_EPSILON) {
		dval = dval / (f->b0 + f->b1 + f->b2);
	}

	f->delay_element_1 = dval;
	f->delay_element_2 = dval;

	return NotchFilter_Apply(f, sample);
}
