/* 
	弹道补偿算法。
	
	。
*/

#pragma once


typedef struct {
	int place_holder;
} Ballistics_t;

void Ballistics_Init(Ballistics_t *b, float sample_freq, float notch_freq, float bandwidth);
float Ballistics_Apply(Ballistics_t *b, float sample);
float Ballistics_Reset(Ballistics_t *b, float sample);
