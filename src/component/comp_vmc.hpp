#pragma once

#include <component.hpp>

namespace Component
{
class VMC
{
 public:
    typedef struct
    {
        float leg_4;       /*前大腿*/
        float leg_1;       /*后大腿*/
        float leg_3;       /*前小腿*/
        float leg_2;       /*后小腿*/
        float hip_length;  /*髋长度*/
    }Param;

    VMC(Param &param, float sample_freq);

    std::tuple<float,float,float> VMCsolve(float phi1 ,float phi4,float eulrPit,float d_eulrPit, float dt);

    std::tuple<float,float> VMCinserve(float phi1 ,float phi2, float Tp, float F0 );

    float LQR_K_calc(float *coe,float len);

    void Reset();

 private:
    Param param_;

 struct
    {


	float XB,YB;     //B点的坐标
	float XD,YD;     //D点的坐标

	float XC,YC;     //C点的直角坐标
	float L0,phi0;   //C点的极坐标
	float alpha;
	float d_alpha;

	float lBD;       //BD两点的距离

	float d_phi0;    //现在C点角度phi0的变换率
	float last_phi0; //上一次C点角度，用于计算角度phi0的变换率d_phi0

	float A0,B0,C0;  //中间变量
	float phi2,phi3;

	float j11,j12,j21,j22; //笛卡尔空间力到关节空间的力的雅可比矩阵系数
	float torque_set[2];

	float theta;
	float d_theta;   //theta的一阶导数
    float last_d_theta;
    float dd_theta;  //theta的二阶导数

	float d_L0;      //L0的一阶导数
	float dd_L0;     //L0的二阶导数
	float last_L0;
    float last_d_L0;

	uint8_t first_flag;
	uint8_t leg_flag; //腿长完成标志

    } vmc_leg;

};
} // namespace Component
