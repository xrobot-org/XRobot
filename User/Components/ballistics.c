/* 
	剩余电流算法。
	
	通过电压值计算剩余电量。
*/

#include "ballistics.h"


void Ballistics_Init(Ballistics_t *b, float sample_freq, float notch_freq, float bandwidth) {

}

inline float Ballistics_Apply(Ballistics_t *b, float sample) {
	return 0.f;
}

float Ballistics_Reset(Ballistics_t *b, float sample) {
	return 0.f;
}
