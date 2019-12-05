#pragma once

typedef struct {
	float kp;
	float ki;
	float kd;
	float set;
	float get;
	float err;
	
} KarmanFilter_t;

void KarmanFilter_Init(KarmanFilter_t* hahrs, float sample_freq, float kp, float ki, float kd);
void KarmanFilter_Update(KarmanFilter_t* hahrs);

