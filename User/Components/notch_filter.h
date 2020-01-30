/* 
	带阻滤波器。

*/

typedef struct {
	float notch_freq;
	float bandwidth;
	
	float a1;
	float a2;

	float b0;
	float b1;
	float b2;
	float delay_element_1;
	float delay_element_2;
		
} NotchFilter_t;

void NotchFilter_SetParameters(NotchFilter_t *f, float sample_freq, float notch_freq, float bandwidth);
float NotchFilter_Apply(NotchFilter_t *f, float sample);
float NotchFilter_Reset(NotchFilter_t *f, float sample);
