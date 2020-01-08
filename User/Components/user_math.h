#pragma once

#include "stm32f427xx.h"
#include <arm_math.h>


float InvSqrt(float x);

float AbsClip(float in, float limit);

float Sign(float in);
