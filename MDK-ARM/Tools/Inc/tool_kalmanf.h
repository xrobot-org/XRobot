#ifndef __TOOLS_KALMANF__H
#define __TOOLS_KALMANF__H

typedef struct {
	float kp;
	float ki;
	float kd;
	float set;
	float get;
	float err;
	
} KarmanFilter_HandleTypeDef;

void KarmanFilter_Init(KarmanFilter_HandleTypeDef *hahrs, float sample_freq, float kp, float ki, float kd);
void KarmanFilter_Update(KarmanFilter_HandleTypeDef *hahrs);

#endif
