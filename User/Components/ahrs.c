/* 
	开源的AHRS算法。
	MadgwickAHRS
*/

#include "ahrs.h"

#include "user_math.h"

#define BETA .033f

/* 2 * proportional gain (Kp) */
static float beta = BETA;

static int AHRS_UpdateRotMatrix(AHRS_t *ahrs) {
	if (ahrs == NULL)
		return -1;
	
	const float q0 = ahrs->q0;
	const float q1 = ahrs->q1;
	const float q2 = ahrs->q2;
	const float q3 = ahrs->q3;
	
	ahrs->rot_matrix[0][0] = 2.f * q0 * q0 - 1.f + 2.f * q1 * q1;
	ahrs->rot_matrix[0][1] = 2.f * (q1 * q2 + q0 * q3);
	ahrs->rot_matrix[0][2] = 2.f * (q1 * q3 - q0 * q2);
	ahrs->rot_matrix[1][0] = 2.f * (q1 * q2 - q0 * q3);
	ahrs->rot_matrix[1][1] = 2.f * q0 * q0 - 1.f + 2.f * q2 * q2;
	ahrs->rot_matrix[1][2] = 2.f * (q2 * q3 + q0 * q1);
	ahrs->rot_matrix[2][0] = 2.f * (q1 * q3 + q0 * q2);
	ahrs->rot_matrix[2][1] = 2.f * (q2 * q3 - q0 * q1);
	ahrs->rot_matrix[2][2] = 2.f * q0 * q0 - 1.f + 2.f * q3 * q3;
	
	return 0;
}

static int AHRS_UpdateEuler(AHRS_t *ahrs) {
	if (ahrs == NULL)
		return -1;
	// 
	ahrs->eulr.yaw = atan2f(ahrs->rot_matrix[0][1], ahrs->rot_matrix[0][0])* 180.f / PI;
	ahrs->eulr.rol = -1.f / sinf(ahrs->rot_matrix[2][0]) * 180.f / PI;
	ahrs->eulr.pit = atan2f(ahrs->rot_matrix[2][1], ahrs->rot_matrix[2][2])* 180.f / PI;
	
	return 0;
}

static int AHRS_UpdateIMU(AHRS_t* ahrs, const IMU_t* imu) {
	if (ahrs == NULL || imu == NULL)
		return -1;
	
	float ax = imu->data.accl.x;
	float ay = imu->data.accl.y;
	float az = imu->data.accl.z;
	
	float gx = imu->data.gyro.x;
	float gy = imu->data.gyro.y;
	float gz = imu->data.gyro.z;
	
	float q0 = ahrs->q0;
	float q1 = ahrs->q1;
	float q2 = ahrs->q2;
	float q3 = ahrs->q3;
	
	float recip_norm;
	float s0, s1, s2, s3;
	float q_dot1, q_dot2, q_dot3, q_dot4;
	float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2 ,_8q1, _8q2, q0q0, q1q1, q2q2, q3q3;

	// Rate of change of quaternion from gyroscope
	q_dot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
	q_dot2 = 0.5f * (q0 * gx + q2 * gz - q3 * gy);
	q_dot3 = 0.5f * (q0 * gy - q1 * gz + q3 * gx);
	q_dot4 = 0.5f * (q0 * gz + q1 * gy - q2 * gx);

	// Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation)
	if(!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {

		// Normalise accelerometer measurement
		recip_norm = InvSqrt(ax * ax + ay * ay + az * az);
		ax *= recip_norm;
		ay *= recip_norm;
		az *= recip_norm;   

		// Auxiliary variables to avoid repeated arithmetic
		_2q0 = 2.0f * q0;
		_2q1 = 2.0f * q1;
		_2q2 = 2.0f * q2;
		_2q3 = 2.0f * q3;
		_4q0 = 4.0f * q0;
		_4q1 = 4.0f * q1;
		_4q2 = 4.0f * q2;
		_8q1 = 8.0f * q1;
		_8q2 = 8.0f * q2;
		q0q0 = q0 * q0;
		q1q1 = q1 * q1;
		q2q2 = q2 * q2;
		q3q3 = q3 * q3;

		// Gradient decent algorithm corrective step
		s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
		s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * q1 - _2q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
		s2 = 4.0f * q0q0 * q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
		s3 = 4.0f * q1q1 * q3 - _2q1 * ax + 4.0f * q2q2 * q3 - _2q2 * ay;
		recip_norm = InvSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3); // normalise step magnitude
		s0 *= recip_norm;
		s1 *= recip_norm;
		s2 *= recip_norm;
		s3 *= recip_norm;

		// Apply feedback step
		q_dot1 -= beta * s0;
		q_dot2 -= beta * s1;
		q_dot3 -= beta * s2;
		q_dot4 -= beta * s3;
	}

	// Integrate rate of change of quaternion to yield quaternion
	q0 += q_dot1 * ahrs->inv_sample_freq;
	q1 += q_dot2 * ahrs->inv_sample_freq;
	q2 += q_dot3 * ahrs->inv_sample_freq;
	q3 += q_dot4 * ahrs->inv_sample_freq;

	// Normalise quaternion
	recip_norm = InvSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
	q0 *= recip_norm;
	q1 *= recip_norm;
	q2 *= recip_norm;
	q3 *= recip_norm;
	
	ahrs->q0 = q0;
	ahrs->q1 = q1;
	ahrs->q2 = q2;
	ahrs->q3 = q3;
	
	AHRS_UpdateRotMatrix(ahrs);
	AHRS_UpdateEuler(ahrs);
	
	return 0;
}

int AHRS_Init(AHRS_t *ahrs, const IMU_t *imu, float sample_freq) {
	if (ahrs == NULL || imu == NULL)
		return -1;
	
	ahrs->inv_sample_freq = 1.0f / sample_freq;
	
#if 1
	if((imu->data.magn.x == 0.0f) && (imu->data.magn.y == 0.0f) && (imu->data.magn.z == 0.0f)) {
		ahrs->q0 = 0.794987798f;
		ahrs->q1 = 0.00132370531f;
		ahrs->q2 = -0.0234376211f;
		ahrs->q3 = 0.608368337f;
	} else {
		ahrs->q0 = 1.f;
		ahrs->q1 = 0.f;
		ahrs->q2 = 0.f;
		ahrs->q3 = 0.f;
	}
#else
	
	ahrs->q0 = 1.f;
	ahrs->q1 = 0.f;
	ahrs->q2 = 0.f;
	ahrs->q3 = 0.f;
	
#endif
	
	return 0;
}

/* Pass the sensor data in a NED(North East Down) reference frame. Rotation can be added. */
int AHRS_Update(AHRS_t *ahrs, const IMU_t *imu) {
	if (ahrs == NULL || imu == NULL)
		return -1;
	
	float recip_norm;
	float s0, s1, s2, s3;
	float q_dot1, q_dot2, q_dot3, q_dot4;
	float hx, hy;
	float _2q0mx, _2q0my, _2q0mz, _2q1mx, _2bx, _2bz, _4bx, _4bz, _2q0, _2q1, _2q2, _2q3, _2q0q2, _2q2q3, q0q0, q0q1, q0q2, q0q3, q1q1, q1q2, q1q3, q2q2, q2q3, q3q3;

	float mx = imu->data.magn.x;
	float my = imu->data.magn.y;
	float mz = imu->data.magn.z;
	
	// Use IMU algorithm if magnetometer measurement invalid (avoids NaN in magnetometer normalisation)
	if((mx == 0.0f) && (my == 0.0f) && (mz == 0.0f)) {
		return AHRS_UpdateIMU(ahrs, imu);
	}
	
	float ax = imu->data.accl.x;
	float ay = imu->data.accl.y;
	float az = imu->data.accl.z;
	
	float gx = imu->data.gyro.x;
	float gy = imu->data.gyro.y;
	float gz = imu->data.gyro.z;
	
	float q0 = ahrs->q0;
	float q1 = ahrs->q1;
	float q2 = ahrs->q2;
	float q3 = ahrs->q3;

	// Rate of change of quaternion from gyroscope
	q_dot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
	q_dot2 = 0.5f * (q0 * gx + q2 * gz - q3 * gy);
	q_dot3 = 0.5f * (q0 * gy - q1 * gz + q3 * gx);
	q_dot4 = 0.5f * (q0 * gz + q1 * gy - q2 * gx);

	// Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation)
	if(!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {

		// Normalise accelerometer measurement
		recip_norm = InvSqrt(ax * ax + ay * ay + az * az);
		ax *= recip_norm;
		ay *= recip_norm;
		az *= recip_norm;   

		// Normalise magnetometer measurement
		recip_norm = InvSqrt(mx * mx + my * my + mz * mz);
		mx *= recip_norm;
		my *= recip_norm;
		mz *= recip_norm;

		// Auxiliary variables to avoid repeated arithmetic
		_2q0mx = 2.0f * q0 * mx;
		_2q0my = 2.0f * q0 * my;
		_2q0mz = 2.0f * q0 * mz;
		_2q1mx = 2.0f * q1 * mx;
		_2q0 = 2.0f * q0;
		_2q1 = 2.0f * q1;
		_2q2 = 2.0f * q2;
		_2q3 = 2.0f * q3;
		_2q0q2 = 2.0f * q0 * q2;
		_2q2q3 = 2.0f * q2 * q3;
		q0q0 = q0 * q0;
		q0q1 = q0 * q1;
		q0q2 = q0 * q2;
		q0q3 = q0 * q3;
		q1q1 = q1 * q1;
		q1q2 = q1 * q2;
		q1q3 = q1 * q3;
		q2q2 = q2 * q2;
		q2q3 = q2 * q3;
		q3q3 = q3 * q3;

		// Reference direction of Earth's magnetic field
		hx = mx * q0q0 - _2q0my * q3 + _2q0mz * q2 + mx * q1q1 + _2q1 * my * q2 + _2q1 * mz * q3 - mx * q2q2 - mx * q3q3;
		hy = _2q0mx * q3 + my * q0q0 - _2q0mz * q1 + _2q1mx * q2 - my * q1q1 + my * q2q2 + _2q2 * mz * q3 - my * q3q3;
		_2bx = sqrt(hx * hx + hy * hy);
		_2bz = -_2q0mx * q2 + _2q0my * q1 + mz * q0q0 + _2q1mx * q3 - mz * q1q1 + _2q2 * my * q3 - mz * q2q2 + mz * q3q3;
		_4bx = 2.0f * _2bx;
		_4bz = 2.0f * _2bz;

		// Gradient decent algorithm corrective step
		s0 = -_2q2 * (2.0f * q1q3 - _2q0q2 - ax) + _2q1 * (2.0f * q0q1 + _2q2q3 - ay) - _2bz * q2 * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (-_2bx * q3 + _2bz * q1) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + _2bx * q2 * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
		s1 = _2q3 * (2.0f * q1q3 - _2q0q2 - ax) + _2q0 * (2.0f * q0q1 + _2q2q3 - ay) - 4.0f * q1 * (1 - 2.0f * q1q1 - 2.0f * q2q2 - az) + _2bz * q3 * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (_2bx * q2 + _2bz * q0) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + (_2bx * q3 - _4bz * q1) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
		s2 = -_2q0 * (2.0f * q1q3 - _2q0q2 - ax) + _2q3 * (2.0f * q0q1 + _2q2q3 - ay) - 4.0f * q2 * (1 - 2.0f * q1q1 - 2.0f * q2q2 - az) + (-_4bx * q2 - _2bz * q0) * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (_2bx * q1 + _2bz * q3) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + (_2bx * q0 - _4bz * q2) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
		s3 = _2q1 * (2.0f * q1q3 - _2q0q2 - ax) + _2q2 * (2.0f * q0q1 + _2q2q3 - ay) + (-_4bx * q3 + _2bz * q1) * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (-_2bx * q0 + _2bz * q2) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + _2bx * q1 * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
		recip_norm = InvSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3); // normalise step magnitude
		s0 *= recip_norm;
		s1 *= recip_norm;
		s2 *= recip_norm;
		s3 *= recip_norm;

		// Apply feedback step
		q_dot1 -= beta * s0;
		q_dot2 -= beta * s1;
		q_dot3 -= beta * s2;
		q_dot4 -= beta * s3;
	}

	// Integrate rate of change of quaternion to yield quaternion
	q0 += q_dot1 * ahrs->inv_sample_freq;
	q1 += q_dot2 * ahrs->inv_sample_freq;
	q2 += q_dot3 * ahrs->inv_sample_freq;
	q3 += q_dot4 * ahrs->inv_sample_freq;

	// Normalise quaternion
	recip_norm = InvSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
	q0 *= recip_norm;
	q1 *= recip_norm;
	q2 *= recip_norm;
	q3 *= recip_norm;
	
	ahrs->q0 = q0;
	ahrs->q1 = q1;
	ahrs->q2 = q2;
	ahrs->q3 = q3;
	
	AHRS_UpdateRotMatrix(ahrs);
	AHRS_UpdateEuler(ahrs);
	return 0;
}
