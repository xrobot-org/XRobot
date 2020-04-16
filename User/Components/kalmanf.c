/* 
	卡尔曼滤波算法。
	
*/

#include "main.h"
#include "kalmanf.h"

void KalmanFilter_Init(KalmanFilter_t *kal){
	/* We will set the variables like so, these can also be tuned by the user */
	kal->q_angle = 0.001;
	kal->q_bias = 0.003;
	kal->r_measure = 0.03;

	kal->angle = 0; // Reset the angle
	kal->bias = 0; // Reset bias

	kal->p[0][0] = 0; // Since we assume that the bias is 0 and we know the starting angle (use setAngle), the error covariance matrix is set like so - see: http://en->wikipedia->org/wiki/Kalman_filter#Example_application->2C_technical
	kal->p[0][1] = 0;
	kal->p[1][0] = 0;
	kal->p[1][1] = 0;
	
}

double KalmanFilter_Update(KalmanFilter_t *kal, double newAngle, double newRate, double dt) {
	kal->rate = newRate - kal->bias;
	kal->angle += dt * kal->rate;

	kal->p[0][0] += dt * (dt*kal->p[1][1] - kal->p[0][1] - kal->p[1][0] + kal->q_angle);
	kal->p[0][1] -= dt * kal->p[1][1];
	kal->p[1][0] -= dt * kal->p[1][1];
	kal->p[1][1] += kal->q_bias * dt;

	kal->s = kal->p[0][0] + kal->r_measure;
	
	kal->k[0] = kal->p[0][0] / kal->s;
	kal->k[1] = kal->p[1][0] / kal->s;
   
	kal->y = newAngle - kal->angle;

	kal->angle += kal->k[0] * kal->y;
	kal->bias += kal->k[1] * kal->y;
   
	kal->p[0][0] -= kal->k[0] * kal->p[0][0];
	kal->p[0][1] -= kal->k[0] * kal->p[0][1];
	kal->p[1][0] -= kal->k[1] * kal->p[0][0];
	kal->p[1][1] -= kal->k[1] * kal->p[0][1];
   
	return kal->angle;
}
