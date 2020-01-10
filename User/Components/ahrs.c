/* 
	开源的AHRS算法。
	
	考虑使用Botch的BSXlite 

*/

#include "ahrs.h"

#include "user_math.h"

#define TWO_KP 1.f
#define TWO_KI 1.f

/* 2 * proportional gain (Kp) */
static float two_kp = TWO_KP;

/* 2 * integral gain (Ki) */
static float two_ki = TWO_KI;

/* integral error terms scaled by Ki */
static float integral_fb_x = 0.f,  integral_fb_y = 0.f, integral_fb_z = 0.f; 


static void AHRS_UpdateEuler(AHRS_t* hahrs) {
	if (hahrs == NULL)
		return;
	
	float q0 = hahrs->q0;
	float q1 = hahrs->q1;
	float q2 = hahrs->q2;
	float q3 = hahrs->q3;
	
	hahrs->rot_matrix[0][0] = q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3;
	hahrs->rot_matrix[0][1] = 2.f * (q1 * q2 + q0 * q3);
	hahrs->rot_matrix[0][2] = 2.f * (q1 * q3 - q0 * q2);
	hahrs->rot_matrix[1][0] = 2.f * (q1 * q2 - q0 * q3);
	hahrs->rot_matrix[1][1] = q0 * q0 - q1 * q1 + q2 * q2 - q3 * q3;
	hahrs->rot_matrix[1][2] = 2.f * (q2 * q3 + q0 * q1);
	hahrs->rot_matrix[2][0] = 2.f * (q1 * q3 + q0 * q2);
	hahrs->rot_matrix[2][1] = 2.f * (q2 * q3 - q0 * q1);
	hahrs->rot_matrix[2][2] = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;
	
	hahrs->eulr.rol = atan2f(hahrs->rot_matrix[1][2], hahrs->rot_matrix[2][2]) * 180.f / PI;
	hahrs->eulr.pit = asinf(hahrs->rot_matrix[0][2]) * 180.f / PI;
	hahrs->eulr.yaw = atan2f(hahrs->rot_matrix[0][1], hahrs->rot_matrix[0][0]) * 180.f / PI;
}

static void AHRS_UpdateIMU(AHRS_t* hahrs, const IMU_t* himu) {
	if (hahrs == NULL || himu == NULL)
		return;
	
	float ax = himu->data.accl.x;
	float ay = himu->data.accl.y;
	float az = himu->data.accl.z;
	
	float gx = himu->data.gyro.x;
	float gy = himu->data.gyro.y;
	float gz = himu->data.gyro.z;
	
	
	float q0 = hahrs->q0;
	float q1 = hahrs->q1;
	float q2 = hahrs->q2;
	float q3 = hahrs->q3;
	
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
			integral_fb_x += two_ki * halfex * hahrs->inv_sample_freq;
			integral_fb_y += two_ki * halfey * hahrs->inv_sample_freq;
			integral_fb_z += two_ki * halfez * hahrs->inv_sample_freq;
			
			/* apply integral feedback */
			gx += integral_fb_x;
			gy += integral_fb_y;
			gz += integral_fb_z;
		} else {
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
	
	#else
	
	float dq0 = 0.5f*(-q1 * gx - q2 * gy - q3 * gz);
	float dq1 = 0.5f*(q0 * gx + q2 * gz - q3 * gy);
	float dq2 = 0.5f*(q0 * gy - q1 * gz + q3 * gx);
	float dq3 = 0.5f*(q0 * gz + q1 * gy - q2 * gx); 

	hahrs->q0 += hahrs->inv_sample_freq * dq0;
	hahrs->q1 += hahrs->inv_sample_freq * dq1;
	hahrs->q2 += hahrs->inv_sample_freq * dq2;
	hahrs->q3 += hahrs->inv_sample_freq * dq3;
	
	#endif
	
	/* Normalise quaternion */
	float recip_norm = InvSqrt(hahrs->q0 * hahrs->q0 + hahrs->q1 * hahrs->q1 + hahrs->q2 * hahrs->q2 + hahrs->q3 * hahrs->q3);
	hahrs->q0 *= recip_norm;
	hahrs->q1 *= recip_norm;
	hahrs->q2 *= recip_norm;
	hahrs->q3 *= recip_norm;
	
	AHRS_UpdateEuler(hahrs);
}

void AHRS_Init(AHRS_t* hahrs, const IMU_t* himu, float sample_freq) {
	if (hahrs == NULL || himu == NULL)
		return;
	
	hahrs->inv_sample_freq = 1.0f / sample_freq;
	
	
	if((himu->data.magn.x == 0.0f) && (himu->data.magn.y == 0.0f) && (himu->data.magn.z == 0.0f)) {
		hahrs->q0 = 0.794987798f;
		hahrs->q1 = 0.00132370531f;
		hahrs->q2 = -0.0234376211f;
		hahrs->q3 = 0.608368337f;
	}
}

/* Pass the sensor data in a NED(North East Down) reference frame. Rotation can be added. */
void AHRS_Update(AHRS_t* hahrs, const IMU_t* himu) {
	if (hahrs == NULL || himu == NULL)
		return;
	
	float ax = himu->data.accl.x;
	float ay = himu->data.accl.y;
	float az = himu->data.accl.z;
	
	float gx = himu->data.gyro.x;
	float gy = himu->data.gyro.y;
	float gz = himu->data.gyro.z;
	
	float mx = himu->data.magn.z;
	float my = himu->data.magn.z;
	float mz = himu->data.magn.z;
	
	float q0 = hahrs->q0;
	float q1 = hahrs->q1;
	float q2 = hahrs->q2;
	float q3 = hahrs->q3;
	
	
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
			integral_fb_x += two_ki * halfex * hahrs->inv_sample_freq;
			integral_fb_y += two_ki * halfey * hahrs->inv_sample_freq;
			integral_fb_z += two_ki * halfez * hahrs->inv_sample_freq;
			
			/* apply integral feedback */
			gx += integral_fb_x;
			gy += integral_fb_y;
			gz += integral_fb_z;
		} else {
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
	
	#else
	
	float dq0 = 0.5f*(-q1 * gx - q2 * gy - q3 * gz);
	float dq1 = 0.5f*(q0 * gx + q2 * gz - q3 * gy);
	float dq2 = 0.5f*(q0 * gy - q1 * gz + q3 * gx);
	float dq3 = 0.5f*(q0 * gz + q1 * gy - q2 * gx); 

	hahrs->q0 += hahrs->inv_sample_freq * dq0;
	hahrs->q1 += hahrs->inv_sample_freq * dq1;
	hahrs->q2 += hahrs->inv_sample_freq * dq2;
	hahrs->q3 += hahrs->inv_sample_freq * dq3;
	
	#endif 
	
	/* Normalise quaternion */
	recip_norm = InvSqrt(hahrs->q0 * hahrs->q0 + hahrs->q1 * hahrs->q1 + hahrs->q2 * hahrs->q2 + hahrs->q3 * hahrs->q3);
	hahrs->q0 *= recip_norm;
	hahrs->q1 *= recip_norm;
	hahrs->q2 *= recip_norm;
	hahrs->q3 *= recip_norm;
	
	AHRS_UpdateEuler(hahrs);
}
