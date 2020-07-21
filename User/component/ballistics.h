/* 
	弹道补偿算法。
	
	。
*/

#pragma once

#include "ahrs.h"

typedef struct {
	AHRS_Eulr_t *eulr;
} Ballistics_t;

void Ballistics_Init(Ballistics_t *b);
void Ballistics_Apply(Ballistics_t *b, float bullet_speed);
void Ballistics_Reset(Ballistics_t *b);
