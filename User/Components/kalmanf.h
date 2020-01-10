/* 
	卡尔曼滤波算法。

*/

#pragma once

typedef struct {

} KarmanFilter_t;

void KarmanFilter_Init(KarmanFilter_t* hahrs, float sample_freq, float kp, float ki, float kd);
void KarmanFilter_Update(KarmanFilter_t* hahrs);

