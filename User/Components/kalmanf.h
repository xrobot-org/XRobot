/* 
	卡尔曼滤波算法。

*/

#pragma once

typedef struct {
	/* Kalman filter variables */
	double q_angle; // Process noise variance for the accelerometer
	double q_bias; // Process noise variance for the gyro bias
	double r_measure; // Measurement noise variance - this is actually the variance of the measurement noise
	double angle; // The angle calculated by the Kalman filter - part of the 2x1 state vector
	double bias; // The gyro bias calculated by the Kalman filter - part of the 2x1 state vector
	double rate; // Unbiased rate calculated from the rate and the calculated bias - you have to call getAngle to update the rate
	double p[2][2]; // Error covariance matrix - This is a 2x2 matrix
	double k[2]; // Kalman gain - This is a 2x1 vector
	double y; // Angle difference
	double s; // Estimate error
} KalmanFilter_t;


void KalmanFilter_Init(KalmanFilter_t *kal);
double KalmanFilter_Update(KalmanFilter_t *kal, double newAngle, double newRate, double dt);
