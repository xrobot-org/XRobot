/* 
	卡尔曼滤波算法。

*/

#pragma once

typedef struct {
	/* Kalman filter variables */
	double Q_angle; // Process noise variance for the accelerometer
	double Q_bias; // Process noise variance for the gyro bias
	double R_measure; // Measurement noise variance - this is actually the variance of the measurement noise

	double angle; // The angle calculated by the Kalman filter - part of the 2x1 state vector
	double bias; // The gyro bias calculated by the Kalman filter - part of the 2x1 state vector
	double rate; // Unbiased rate calculated from the rate and the calculated bias - you have to call getAngle to update the rate

	double P[2][2]; // Error covariance matrix - This is a 2x2 matrix
	double K[2]; // Kalman gain - This is a 2x1 vector
	double y; // Angle difference
	double S; // Estimate error
} KalmanFilter_t;


void KalmanFilter_Init(KalmanFilter_t* hahrs);
double KalmanFilter_Update(KalmanFilter_t* hahrs, double newAngle, double newRate, double dt);

void setAngle(KalmanFilter_t *hahrs, double newAngle);
double getRate(KalmanFilter_t *hahrs); 

void setQangle(KalmanFilter_t *hahrs, double newQ_angle);
void setQbias(KalmanFilter_t *hahrs, double newQ_bias);
void setRmeasure(KalmanFilter_t *hahrs, double newR_measure);
double getQangle(KalmanFilter_t *hahrs);
double getQbias(KalmanFilter_t *hahrs);
double getRmeasure(KalmanFilter_t *hahrs);
