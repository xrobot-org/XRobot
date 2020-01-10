/* 
	舵机控制算法。
	
	HAL TIM部分
	参考2020官方步兵开源 bsp_servo_pwm.c/h。
	
	
	不要用 __HAL_TIM_SetCompare
	参考pwm.c的写法，把pwm中的A~Z全挪到这里来
	
	耿文韬 1.12
*/

#include "servo.h"
#include "main.h"

#include "tim.h"
