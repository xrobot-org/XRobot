/* 
	剩余电流算法。
	
	通过电压值计算剩余电量。
*/

#pragma once


typedef struct {
	
} Ballistics_t;

void Ballistics_Init(Ballistics_t *b, float sample_freq, float notch_freq, float bandwidth);
float Ballistics_Apply(Ballistics_t *b, float sample);
float Ballistics_Reset(Ballistics_t *b, float sample);
