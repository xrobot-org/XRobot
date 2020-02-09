/* 
	
*/


int Mixer_Mecanum(float vx, float vy, float wz, float *out, int len) {
	if (len == 4) {
		out[0] = -vx - vy + wz;
		out[1] = vx - vy + wz;
		out[2] = vx + vy + wz;
		out[3] = -vx + vy + wz;
		return 0;
	} else {
		return -1;
	}
}

int Mixer_ParlFix4(float vx, float vy, float wz, float *out, int len) {
	if (len == 4) {
		out[0] = -vx;
		out[1] = vx;
		out[2] = vx;
		out[3] = -vx;
		return 0;
	} else {
		return -1;
	}
}

int Mixer_ParlFix2(float vx, float vy, float wz, float *out, int len) {
	if (len == 2) {
		out[0] = -vx;
		out[1] = vx;
		return 0;
	} else {
		return -1;
	}
}

int Mixer_OmniCross(float vx, float vy, float wz, float *out, int len) {
	return 0;
}

int Mixer_OmniPlus(float vx, float vy, float wz, float *out, int len) {
	return 0;
}
