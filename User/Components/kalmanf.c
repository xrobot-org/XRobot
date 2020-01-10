/* 
	卡尔曼滤波算法。
	
	马志远-1.12
*/
#include "main.h"
#include "kalmanf.h"

/**
 *@function KarmanFilter_Init
 *@detail		初始化卡尔曼滤波器
 *@para	hahrs	卡尔曼结构体
 */
void KalmanFilter_Init(KalmanFilter_t* hahrs){
	/* We will set the variables like so, these can also be tuned by the user */
    hahrs->Q_angle = 0.001;
    hahrs->Q_bias = 0.003;
    hahrs->R_measure = 0.03;

    hahrs->angle = 0; // Reset the angle
    hahrs->bias = 0; // Reset bias

    hahrs->P[0][0] = 0; // Since we assume that the bias is 0 and we know the starting angle (use setAngle), the error covariance matrix is set like so - see: http://en->wikipedia->org/wiki/Kalman_filter#Example_application->2C_technical
    hahrs->P[0][1] = 0;
    hahrs->P[1][0] = 0;
    hahrs->P[1][1] = 0;
	
}


/**
	*@function KalmanFilter_Update		更新卡尔曼滤波器
	*@para	hahrs			卡尔曼滤波器结构体
	*@para	newAngle	测得的角度		度
	*@para	newRate		测得的角速度	度/秒
	*@para	dt				周期
	*@return	卡尔曼滤波后的角度
	*/
double KalmanFilter_Update(KalmanFilter_t *hahrs, double newAngle, double newRate, double dt) {
    hahrs->rate = newRate - hahrs->bias;
    hahrs->angle += dt * hahrs->rate;

    hahrs->P[0][0] += dt * (dt*hahrs->P[1][1] - hahrs->P[0][1] - hahrs->P[1][0] + hahrs->Q_angle);
    hahrs->P[0][1] -= dt * hahrs->P[1][1];
    hahrs->P[1][0] -= dt * hahrs->P[1][1];
    hahrs->P[1][1] += hahrs->Q_bias * dt;

    hahrs->S = hahrs->P[0][0] + hahrs->R_measure;
    
    hahrs->K[0] = hahrs->P[0][0] / hahrs->S;
    hahrs->K[1] = hahrs->P[1][0] / hahrs->S;
   
    hahrs->y = newAngle - hahrs->angle;

    hahrs->angle += hahrs->K[0] * hahrs->y;
    hahrs->bias += hahrs->K[1] * hahrs->y;
   
    hahrs->P[0][0] -= hahrs->K[0] * hahrs->P[0][0];
    hahrs->P[0][1] -= hahrs->K[0] * hahrs->P[0][1];
    hahrs->P[1][0] -= hahrs->K[1] * hahrs->P[0][0];
    hahrs->P[1][1] -= hahrs->K[1] * hahrs->P[0][1];
   
    return hahrs->angle;
}

void setAngle(KalmanFilter_t *hahrs, double newAngle) { hahrs->angle = newAngle; } // 设置角度，初始值可能会用到
double getRate(KalmanFilter_t *hahrs) { return hahrs->rate; } // Return the unbiased rate

//获取调整卡尔曼滤波器参数
void setQangle(KalmanFilter_t *hahrs, double newQ_angle) { hahrs->Q_angle = newQ_angle; }
void setQbias(KalmanFilter_t *hahrs, double newQ_bias) { hahrs->Q_bias = newQ_bias; }
void setRmeasure(KalmanFilter_t *hahrs, double newR_measure) { hahrs->R_measure = newR_measure; }

double getQangle(KalmanFilter_t *hahrs) { return hahrs->Q_angle; }
double getQbias(KalmanFilter_t *hahrs) { return hahrs->Q_bias; }
double getRmeasure(KalmanFilter_t *hahrs) { return hahrs->R_measure; }
