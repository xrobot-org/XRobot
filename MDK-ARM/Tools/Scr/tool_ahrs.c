#include "tool_ahrs.h"

#include <math.h>

#define M_PI 3.141592653589793238462643383f

/* USE_MAHONY or USE_MADGWICK must be difined. */
#define USE_MAHONY

/* For fater converge */
volatile float q0 = 0.794987798f;
volatile float q1 = 0.00132370531f;
volatile float q2 = -0.0234376211f;
volatile float q3 = 0.608368337f;

static volatile float rot_matrix[3][3] = {0.f};

static float inv_sample_freq = 1.f;

#ifdef USE_MADGWICK

static volatile float beta= 0.041f;

#elif defined USE_MAHONY

/* 2 * proportional gain (Kp) */
static volatile float two_kp = (2.0f * 0.5f);

/* 2 * integral gain (Ki) */
static volatile float two_ki = (2.0f * 0.0f);

/* integral error terms scaled by Ki */
static volatile float integral_fb_x = 0.0f,  integral_fb_y = 0.0f, integral_fb_z = 0.0f; 

#endif

/* Fast inverse square-root
 * See: http://en.wikipedia.org/wiki/Fast_inverse_square_root
 */
static float InvSqrt(float x) {
	float halfx = 0.5f * x;
	float y = x;
	long i = *(long*)&y;
	i = 0x5f3759df - (i>>1);
	y = *(float*)&i;
	y = y * (1.5f - (halfx * y * y));
	return y;
}

static void AHRS_UpdateEuler(AHRS_HandleTypeDef *hahrs) {
	rot_matrix[0][0] = q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3;
	rot_matrix[0][1] = 2.f * (q1 * q2 + q0 * q3);
	rot_matrix[0][2] = 2.f * (q1 * q3 - q0 * q2);
	rot_matrix[1][0] = 2.f * (q1 * q2 - q0 * q3);
	rot_matrix[1][1] = q0 * q0 - q1 * q1 + q2 * q2 - q3 * q3;
	rot_matrix[1][2] = 2.f * (q2 * q3 + q0 * q1);
	rot_matrix[2][0] = 2.f * (q1 * q3 + q0 * q2);
	rot_matrix[2][1] = 2.f * (q2 * q3 - q0 * q1);
	rot_matrix[2][2] = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;
	
	#if 0
	/* roll (x-axis rotation) */
	float sinr_cosp = 2.f * (q0 * q1 + q2 * q3);
	float cosr_cosp = 1.f - 2.f * (q1 * q1 + q2 * q2);
	hahrs->eulr.rol = atan2f(sinr_cosp, cosr_cosp) * 180.f / M_PI;

	/* pitch (y-axis rotation) */
	float sinp = 2.f * (q0 * q2 - q3 * q1);
	if (fabsf(sinp) >= 1.f)
		hahrs->eulr.pit = copysignf(M_PI / 2.f, sinp); /* use 90 degrees if out of range */
	else
		hahrs->eulr.pit = asinf(sinp);
	
	hahrs->eulr.pit *= 180.f / M_PI;

	/* yaw (z-axis rotation) */
	float siny_cosp = 2.f * (q0 * q3 + q1 * q2);
	float cosy_cosp = 1.f - 2.f * (q2 * q2 + q3 * q3);  
	hahrs->eulr.yaw = atan2f(siny_cosp, cosy_cosp) * 180.f / M_PI;
	
	#endif
	
	hahrs->eulr.rol = atan2f(rot_matrix[1][2], rot_matrix[2][2]) * 180.f / M_PI;
	hahrs->eulr.pit = asinf(rot_matrix[0][2]) * 180.f / M_PI;
	hahrs->eulr.yaw = atan2f(rot_matrix[0][1], rot_matrix[0][0]) * 180.f / M_PI;
}

/* Modified from Madgwick's code. */
static void AHRS_UpdateIMU(AHRS_HandleTypeDef *hahrs, const IMU_HandleTypeDef *himu) {
	float ax = himu->accl.x;
	float ay = himu->accl.y;
	float az = himu->accl.z;
	
	float gx = himu->gyro.x;
	float gy = himu->gyro.y;
	float gz = himu->gyro.z;
	
	#ifdef USE_MADGWICK
	
	/* Rate of change of quaternion from gyroscope */
	float q_dot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
	float q_dot2 = 0.5f * (q0 * gx + q2 * gz - q3 * gy);
	float q_dot3 = 0.5f * (q0 * gy - q1 * gz + q3 * gx);
	float q_dot4 = 0.5f * (q0 * gz + q1 * gy - q2 * gx);

	/* Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation) */
	if(!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {

		/* Normalise accelerometer measurement */
		float recip_norm = InvSqrt(ax * ax + ay * ay + az * az);
		ax *= recip_norm;
		ay *= recip_norm;
		az *= recip_norm;   

		/* Auxiliary variables to avoid repeated arithmetic */
		float two_q0 = 2.0f * q0;
		float two_q1 = 2.0f * q1;
		float two_q2 = 2.0f * q2;
		float two_q3 = 2.0f * q3;
		float _4q0 = 4.0f * q0;
		float _4q1 = 4.0f * q1;
		float _4q2 = 4.0f * q2;
		float _8q1 = 8.0f * q1;
		float _8q2 = 8.0f * q2;
		float q0q0 = q0 * q0;
		float q1q1 = q1 * q1;
		float q2q2 = q2 * q2;
		float q3q3 = q3 * q3;

		/* Gradient decent algorithm corrective step */
		float s0 = _4q0 * q2q2 + two_q2 * ax + _4q0 * q1q1 - two_q1 * ay;
		float s1 = _4q1 * q3q3 - two_q3 * ax + 4.0f * q0q0 * q1 - two_q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
		float s2 = 4.0f * q0q0 * q2 + two_q0 * ax + _4q2 * q3q3 - two_q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
		float s3 = 4.0f * q1q1 * q3 - two_q1 * ax + 4.0f * q2q2 * q3 - two_q2 * ay;
		
		/* normalise step magnitude */
		recip_norm = InvSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
		s0 *= recip_norm;
		s1 *= recip_norm;
		s2 *= recip_norm;
		s3 *= recip_norm;

		/* Apply feedback step */
		q_dot1 -= beta * s0;
		q_dot2 -= beta * s1;
		q_dot3 -= beta * s2;
		q_dot4 -= beta * s3;
	}

	/* Integrate rate of change of quaternion to yield quaternion */
	q0 += q_dot1 * inv_sample_freq;
	q1 += q_dot2 * inv_sample_freq;
	q2 += q_dot3 * inv_sample_freq;
	q3 += q_dot4 * inv_sample_freq;

	/* Normalise quaternion */
	float recip_norm = InvSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
	q0 *= recip_norm;
	q1 *= recip_norm;
	q2 *= recip_norm;
	q3 *= recip_norm;
	
	#elif defined USE_MAHONY

	/* Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation) */
	if(!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {

		/* Normalise accelerometer measurement */
		float recip_norm = InvSqrt(ax * ax + ay * ay + az * az);
		ax *= recip_norm;
		ay *= recip_norm;
		az *= recip_norm;        

		/* Estimated direction of gravity and vector perpendicular to magnetic flux */
		float halfvx = q1 * q3 - q0 * q2;
		float halfvy = q0 * q1 + q2 * q3;
		float halfvz = q0 * q0 - 0.5f + q3 * q3;
	
		/* Error is sum of cross product between estimated and measured direction of gravity */
		float halfex = (ay * halfvz - az * halfvy);
		float halfey = (az * halfvx - ax * halfvz);
		float halfez = (ax * halfvy - ay * halfvx);

		/* Compute and apply integral feedback if enabled */
		if(two_ki > 0.0f) {
			/* integral error scaled by Ki */
			integral_fb_x += two_ki * halfex * inv_sample_freq;
			integral_fb_y += two_ki * halfey * inv_sample_freq;
			integral_fb_z += two_ki * halfez * inv_sample_freq;
			
			/* apply integral feedback */
			gx += integral_fb_x;
			gy += integral_fb_y;
			gz += integral_fb_z;
		}
		else {
			integral_fb_x = 0.0f;	/* prevent integral windup */
			integral_fb_y = 0.0f;
			integral_fb_z = 0.0f;
		}

		/* Apply proportional feedback */
		gx += two_kp * halfex;
		gy += two_kp * halfey;
		gz += two_kp * halfez;
	}
	
	/* Integrate rate of change of quaternion */
	
	#if 0
	/* pre-multiply common factors */
	gx *= (0.5f * inv_sample_freq);		
	gy *= (0.5f * inv_sample_freq);
	gz *= (0.5f * inv_sample_freq);
	
	float qa = q0;
	float qb = q1;
	float qc = q2;
	q0 += (-qb * gx - qc * gy - q3 * gz);
	q1 += (qa * gx + qc * gz - q3 * gy);
	q2 += (qa * gy - qb * gz + q3 * gx);
	q3 += (qa * gz + qb * gy - qc * gx); 
	#endif
	
	float dq0 = 0.5f*(-q1 * gx - q2 * gy - q3 * gz);
	float dq1 = 0.5f*(q0 * gx + q2 * gz - q3 * gy);
	float dq2 = 0.5f*(q0 * gy - q1 * gz + q3 * gx);
	float dq3 = 0.5f*(q0 * gz + q1 * gy - q2 * gx); 

	q0 += inv_sample_freq * dq0;
	q1 += inv_sample_freq * dq1;
	q2 += inv_sample_freq * dq2;
	q3 += inv_sample_freq * dq3;
	
	/* Normalise quaternion */
	float recip_norm = InvSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
	q0 *= recip_norm;
	q1 *= recip_norm;
	q2 *= recip_norm;
	q3 *= recip_norm;
	
	#else
	#error USE_MADGWICK or USE_MAHONY should be defined.
	#endif
	
	AHRS_UpdateEuler(hahrs);
}


void AHRS_Init(AHRS_HandleTypeDef *hahrs, float sample_freq){
	inv_sample_freq = 1.0f / sample_freq;
	/* TODO: Add quaternion initiation according to different direction. */
}

/* Modified from Madgwick's code. */
/* Pass the sensor data in a NED(North East Down) reference frame. Rotation can be added. */
void AHRS_Update(AHRS_HandleTypeDef *hahrs, const IMU_HandleTypeDef *himu) {
	float ax = himu->accl.x;
	float ay = himu->accl.y;
	float az = himu->accl.z;
	
	float gx = himu->gyro.x;
	float gy = himu->gyro.y;
	float gz = himu->gyro.z;
	
	float mx = himu->magn.x;
	float my = himu->magn.y;
	float mz = himu->magn.z;
	
	#ifdef USE_MADGWICK

	/* Use IMU algorithm if magnetometer measurement invalid (avoids NaN in magnetometer normalisation) */
	if((mx == 0.0f) && (my == 0.0f) && (mz == 0.0f)) {
		AHRS_UpdateIMU(hahrs, himu);
		return;
	}

	/* Rate of change of quaternion from gyroscope */
	float q_dot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
	float q_dot2 = 0.5f * (q0 * gx + q2 * gz - q3 * gy);
	float q_dot3 = 0.5f * (q0 * gy - q1 * gz + q3 * gx);
	float q_dot4 = 0.5f * (q0 * gz + q1 * gy - q2 * gx);

	/* Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation) */
	if(!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {

		/* Normalise accelerometer measurement */
		float recip_norm = InvSqrt(ax * ax + ay * ay + az * az);
		ax *= recip_norm;
		ay *= recip_norm;
		az *= recip_norm;   

		/* Normalise magnetometer measurement */
		recip_norm = InvSqrt(mx * mx + my * my + mz * mz);
		mx *= recip_norm;
		my *= recip_norm;
		mz *= recip_norm;

		/* Auxiliary variables to avoid repeated arithmetic */
		float two_q0mx = 2.0f * q0 * mx;
		float two_q0my = 2.0f * q0 * my;
		float two_q0mz = 2.0f * q0 * mz;
		float two_q1mx = 2.0f * q1 * mx;
		float two_q0 = 2.0f * q0;
		float two_q1 = 2.0f * q1;
		float two_q2 = 2.0f * q2;
		float two_q3 = 2.0f * q3;
		float two_q0q2 = 2.0f * q0 * q2;
		float two_q2q3 = 2.0f * q2 * q3;
		float q0q0 = q0 * q0;
		float q0q1 = q0 * q1;
		float q0q2 = q0 * q2;
		float q0q3 = q0 * q3;
		float q1q1 = q1 * q1;
		float q1q2 = q1 * q2;
		float q1q3 = q1 * q3;
		float q2q2 = q2 * q2;
		float q2q3 = q2 * q3;
		float q3q3 = q3 * q3;

		/* Reference direction of Earth's magnetic field */
		float hx = mx * q0q0 - two_q0my * q3 + two_q0mz * q2 + mx * q1q1 + two_q1 * my * q2 + two_q1 * mz * q3 - mx * q2q2 - mx * q3q3;
		float hy = two_q0mx * q3 + my * q0q0 - two_q0mz * q1 + two_q1mx * q2 - my * q1q1 + my * q2q2 + two_q2 * mz * q3 - my * q3q3;
		float _2bx = sqrt(hx * hx + hy * hy);
		float _2bz = -two_q0mx * q2 + two_q0my * q1 + mz * q0q0 + two_q1mx * q3 - mz * q1q1 + two_q2 * my * q3 - mz * q2q2 + mz * q3q3;
		float _4bx = 2.0f * _2bx;
		float _4bz = 2.0f * _2bz;

		/* Gradient decent algorithm corrective step */
		float s0 = -two_q2 * (2.0f * q1q3 - two_q0q2 - ax) + two_q1 * (2.0f * q0q1 + two_q2q3 - ay) - _2bz * q2 * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (-_2bx * q3 + _2bz * q1) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + _2bx * q2 * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
		float s1 = two_q3 * (2.0f * q1q3 - two_q0q2 - ax) + two_q0 * (2.0f * q0q1 + two_q2q3 - ay) - 4.0f * q1 * (1 - 2.0f * q1q1 - 2.0f * q2q2 - az) + _2bz * q3 * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (_2bx * q2 + _2bz * q0) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + (_2bx * q3 - _4bz * q1) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
		float s2 = -two_q0 * (2.0f * q1q3 - two_q0q2 - ax) + two_q3 * (2.0f * q0q1 + two_q2q3 - ay) - 4.0f * q2 * (1 - 2.0f * q1q1 - 2.0f * q2q2 - az) + (-_4bx * q2 - _2bz * q0) * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (_2bx * q1 + _2bz * q3) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + (_2bx * q0 - _4bz * q2) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
		float s3 = two_q1 * (2.0f * q1q3 - two_q0q2 - ax) + two_q2 * (2.0f * q0q1 + two_q2q3 - ay) + (-_4bx * q3 + _2bz * q1) * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (-_2bx * q0 + _2bz * q2) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + _2bx * q1 * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
		
		/* normalise step magnitude */
		recip_norm = InvSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
		s0 *= recip_norm;
		s1 *= recip_norm;
		s2 *= recip_norm;
		s3 *= recip_norm;

		/* Apply feedback step */
		q_dot1 -= beta * s0;
		q_dot2 -= beta * s1;
		q_dot3 -= beta * s2;
		q_dot4 -= beta * s3;
	}

	/* Integrate rate of change of quaternion to yield quaternion */
	q0 += q_dot1 * inv_sample_freq;
	q1 += q_dot2 * inv_sample_freq;
	q2 += q_dot3 * inv_sample_freq;
	q3 += q_dot4 * inv_sample_freq;
	
	/* Normalise quaternion */
	float recip_norm = InvSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
	q0 *= recip_norm;
	q1 *= recip_norm;
	q2 *= recip_norm;
	q3 *= recip_norm;
	
	#elif defined USE_MAHONY
	
	/* Use IMU algorithm if magnetometer measurement invalid (avoids NaN in magnetometer normalisation) */
	if((mx == 0.0f) && (my == 0.0f) && (mz == 0.0f)) {
		AHRS_UpdateIMU(hahrs, himu);
		return;
	}
	
	/* Normalise magnetometer measurement */
	float recip_norm = InvSqrt(mx * mx + my * my + mz * mz);
	mx *= recip_norm;
	my *= recip_norm;
	mz *= recip_norm;   

	/* Auxiliary variables to avoid repeated arithmetic */
	float q0q0 = q0 * q0;
	float q0q1 = q0 * q1;
	float q0q2 = q0 * q2;
	float q0q3 = q0 * q3;
	float q1q1 = q1 * q1;
	float q1q2 = q1 * q2;
	float q1q3 = q1 * q3;
	float q2q2 = q2 * q2;
	float q2q3 = q2 * q3;
	float q3q3 = q3 * q3;

	/* Reference direction of Earth's magnetic field */
	float hx = 2.0f * (mx * (0.5f - q2q2 - q3q3) + my * (q1q2 - q0q3) + mz * (q1q3 + q0q2));
	float hy = 2.0f * (mx * (q1q2 + q0q3) + my * (0.5f - q1q1 - q3q3) + mz * (q2q3 - q0q1));
	float bx = sqrt(hx * hx + hy * hy);
	float bz = 2.0f * (mx * (q1q3 - q0q2) + my * (q2q3 + q0q1) + mz * (0.5f - q1q1 - q2q2));
	
	/* Estimated direction of gravity and magnetic field */
	float halfwx = bx * (0.5f - q2q2 - q3q3) + bz * (q1q3 - q0q2);
	float halfwy = bx * (q1q2 - q0q3) + bz * (q0q1 + q2q3);
	float halfwz = bx * (q0q2 + q1q3) + bz * (0.5f - q1q1 - q2q2);  
	
	/* Error is sum of cross product between estimated direction and measured direction of field vectors */
	float halfex = my * halfwz - mz * halfwy;
	float halfey = mz * halfwx - mx * halfwz;
	float halfez = mx * halfwy - my * halfwx;

	/* Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation) */
	if(!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {

		/* Normalise accelerometer measurement */
		recip_norm = InvSqrt(ax * ax + ay * ay + az * az);
		ax *= recip_norm;
		ay *= recip_norm;
		az *= recip_norm;     

		float halfvx = q1q3 - q0q2;
		float halfvy = q0q1 + q2q3;
		float halfvz = q0q0 - 0.5f + q3q3;
		
		/* Error is sum of cross product between estimated direction and measured direction of field vectors */
		halfex += (ay * halfvz - az * halfvy);
		halfey += (az * halfvx - ax * halfvz);
		halfez += (ax * halfvy - ay * halfvx);
	}
	
	if(halfex != 0.0f && halfey != 0.0f && halfez != 0.0f) {
		/* Compute and apply integral feedback if enabled */
		if(two_ki > 0.0f) {
			/* integral error scaled by Ki */
			integral_fb_x += two_ki * halfex * inv_sample_freq;
			integral_fb_y += two_ki * halfey * inv_sample_freq;
			integral_fb_z += two_ki * halfez * inv_sample_freq;
			
			/* apply integral feedback */
			gx += integral_fb_x;
			gy += integral_fb_y;
			gz += integral_fb_z;
		}
		else {
			/* prevent integral windup */
			integral_fb_x = 0.0f;
			integral_fb_y = 0.0f;
			integral_fb_z = 0.0f;
		}

		/* Apply proportional feedback */
		gx += two_kp * halfex;
		gy += two_kp * halfey;
		gz += two_kp * halfez;
	}
	
	/* Integrate rate of change of quaternion */
	
	#if 0
	/* pre-multiply common factors */
	gx *= (0.5f * inv_sample_freq);
	gy *= (0.5f * inv_sample_freq);
	gz *= (0.5f * inv_sample_freq);
	
	float qa = q0;
	float qb = q1;
	float qc = q2;
	q0 += (-qb * gx - qc * gy - q3 * gz);
	q1 += (qa * gx + qc * gz - q3 * gy);
	q2 += (qa * gy - qb * gz + q3 * gx);
	q3 += (qa * gz + qb * gy - qc * gx); 
	#endif 
	
	float dq0 = 0.5f*(-q1 * gx - q2 * gy - q3 * gz);
	float dq1 = 0.5f*(q0 * gx + q2 * gz - q3 * gy);
	float dq2 = 0.5f*(q0 * gy - q1 * gz + q3 * gx);
	float dq3 = 0.5f*(q0 * gz + q1 * gy - q2 * gx); 

	q0 += inv_sample_freq * dq0;
	q1 += inv_sample_freq * dq1;
	q2 += inv_sample_freq * dq2;
	q3 += inv_sample_freq * dq3;
	
	/* Normalise quaternion */
	recip_norm = InvSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
	q0 *= recip_norm;
	q1 *= recip_norm;
	q2 *= recip_norm;
	q3 *= recip_norm;
	
	#else
	#error USE_MADGWICK or USE_MAHONY should be defined.
	#endif
	
	AHRS_UpdateEuler(hahrs);
}
