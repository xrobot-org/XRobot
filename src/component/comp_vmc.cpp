
#include "comp_vmc.hpp"

#include <tuple>

#define PI_2 1.571f

using namespace Component;

VMC::VMC(VMC::Param &param,float sample_freq):
    param_(param)
{
    float dt_min =1.0f /sample_freq;
    XB_ASSERT(isfinite(dt_min));

    this->Reset();
}


/*

 正负极参考韭菜的菜 知乎 平衡步兵控制系统设计

 VMC 机体pitch正负极 d_pitch同

	  /
	 /  正+
	/
x  ---------> 0
	\	负-
	 \
	  \

 phi角正负极  d_phi同

 	  /
	 /  正+
	/
x  ---------> 0
	\	负-
	 \
	  \

    后 <---->前
  /phi1-----phi4\
    /        \
	\        /
	 \  OO  /
	  \O轮O/
		OO

*/





/* 两个大腿角度 机体角度 角速度  求出虚拟腿摆角 摆角速度  虚拟腿长 虚拟腿长变化速度 */
 std::tuple<float,float,float> VMC::VMCsolve(float phi1 ,float phi4,float eulrPit,float d_eulrPit, float dt)
{

	  static float body_pitch=0.0f;
	  static float d_body_pitch=0.0f;
	  body_pitch=eulrPit;
	  d_body_pitch=d_eulrPit;
      /*点D B x y坐标 */
	  this->vmc_leg.YD = this->param_.leg_4 * sinf(phi4);
	  this->vmc_leg.YB = this->param_.leg_1 * sinf(phi1);
	  this->vmc_leg.XD = this->param_.hip_length + this->param_.leg_4 * cosf(phi4);
	  this->vmc_leg.XB = this->param_.leg_1 * cosf(phi1);
      /*BD长度*/
	  this->vmc_leg.lBD = sqrtf((this->vmc_leg.XD - this->vmc_leg.XB)*(this->vmc_leg.XD - this->vmc_leg.XB) + (this->vmc_leg.YD -this->vmc_leg.YB)*(this->vmc_leg.YD - this->vmc_leg.YB));

	  this->vmc_leg.A0 = 2 * this->param_.leg_2 * (this->vmc_leg.XD - this->vmc_leg.XB);
	  this->vmc_leg.B0 = 2 * this->param_.leg_2 * (this->vmc_leg.YD - this->vmc_leg.YB);
	  this->vmc_leg.C0 = this->param_.leg_2 * this->param_.leg_2 + this->vmc_leg.lBD*this->vmc_leg.lBD - this->param_.leg_3 * this->param_.leg_3;
	  this->vmc_leg.phi2 = 2*atan2f((this->vmc_leg.B0 + sqrtf(this->vmc_leg.A0*this->vmc_leg.A0 + this->vmc_leg.B0 * this->vmc_leg.B0 - this->vmc_leg.C0 * this->vmc_leg.C0)),this->vmc_leg.A0 + this->vmc_leg.C0);
	  this->vmc_leg.phi3 = atan2f(this->vmc_leg.YB-this->vmc_leg.YD+this->param_.leg_2*sinf(this->vmc_leg.phi2),this->vmc_leg.XB -this->vmc_leg.XD + this->param_.leg_2 * cosf(this->vmc_leg.phi2));
      /*点C x y坐标 */
	  this->vmc_leg.XC = this->param_.leg_1 * cosf(phi1) + this->param_.leg_2 * cosf(this->vmc_leg.phi2);
	  this->vmc_leg.YC = this->param_.leg_1 * sinf(phi1) + this->param_.leg_2 * sinf(this->vmc_leg.phi2);
      /*点C 极坐标 */
	  this->vmc_leg.L0 = sqrtf((this->vmc_leg.XC - this->param_.hip_length/2.0f) * (this->vmc_leg.XC - this->param_.hip_length/2.0f) + this->vmc_leg.YC * this->vmc_leg.YC);
	  this->vmc_leg.phi0 = atan2f(this->vmc_leg.YC,(this->vmc_leg.XC - this->param_.hip_length/2.0f));

	  this->vmc_leg.alpha=PI_2 - this->vmc_leg.phi0 ;

	//   if(this->vmc_leg.first_flag==0)
	//   {
	//     this->vmc_leg.last_phi0=this->vmc_leg.phi0 ;
	//     this->vmc_leg.first_flag=1;
	//   }

      this->vmc_leg.d_phi0=(this->vmc_leg.phi0 - this->vmc_leg.last_phi0)/dt;
	  this->vmc_leg.d_alpha=0.0f-this->vmc_leg.d_phi0 ;
      /*虚拟腿 摆角theta 摆角速度d_theta */
	  this->vmc_leg.theta = PI_2 + body_pitch-this->vmc_leg.phi0;
	  this->vmc_leg.d_theta=(-d_body_pitch-this->vmc_leg.d_phi0);

      this->vmc_leg.last_phi0 = this->vmc_leg.phi0 ;
      /*虚拟腿 腿长L0 腿长变化速度d_L0 */
	  this->vmc_leg.d_L0=(this->vmc_leg.L0-this->vmc_leg.last_L0)/dt;
      this->vmc_leg.dd_L0=(this->vmc_leg.d_L0-this->vmc_leg.last_d_L0)/dt;

	  this->vmc_leg.last_d_L0=this->vmc_leg.d_L0;
	  this->vmc_leg.last_L0 = this->vmc_leg.L0;

	  this->vmc_leg.dd_theta=(this->vmc_leg.d_theta-this->vmc_leg.last_d_theta)/dt;
	  this->vmc_leg.last_d_theta=this->vmc_leg.d_theta;

      return std::make_tuple(vmc_leg.L0,vmc_leg.theta,vmc_leg.d_theta);

}

/* 两个大腿角度 期望腿支持力 期望腿摆力矩 求出两个关节输出力矩 */
 std::tuple<float,float> VMC::VMCinserve(float phi1,float phi4 ,float Tp,float F0)
{
	    /*jacobian矩阵计算*/
		this->vmc_leg.j11 = (this->param_.leg_1 * sinf(this->vmc_leg.phi0 - this->vmc_leg.phi3) * sinf(phi1 - this->vmc_leg.phi2))/sinf(this->vmc_leg.phi3 - this->vmc_leg.phi2);
		this->vmc_leg.j12 = (this->param_.leg_1 * cosf(this->vmc_leg.phi0 - this->vmc_leg.phi3) * sinf(phi1 - this->vmc_leg.phi2))/(this->vmc_leg.L0 * sinf(this->vmc_leg.phi3  - this->vmc_leg.phi2));
		this->vmc_leg.j21 = (this->param_.leg_4 * sinf(this->vmc_leg.phi0 - this->vmc_leg.phi2) * sinf(this->vmc_leg.phi3 - phi4))/sinf(this->vmc_leg.phi3 - this->vmc_leg.phi2);
		this->vmc_leg.j22 = (this->param_.leg_4 * cosf(this->vmc_leg.phi0 - this->vmc_leg.phi2) * sinf(this->vmc_leg.phi3 - phi4))/(this->vmc_leg.L0 * sinf(this->vmc_leg.phi3 - this->vmc_leg.phi2));
        /*得到前髋关节的输出轴期望力矩，F0为五连杆机构末端沿腿的推力*/
		this->vmc_leg.torque_set[0] = this->vmc_leg.j11 * F0 + this->vmc_leg.j12 * Tp;
	    /*得到后髋关节的输出轴期望力矩，Tp为虚拟腿摆力矩的力矩*/
		this->vmc_leg.torque_set[1] = this->vmc_leg.j21 * F0 + this->vmc_leg.j22 * Tp;

        return std::make_tuple(this->vmc_leg.torque_set[0],this->vmc_leg.torque_set[1]);
}

/* 计算拟合函数结果 */
float VMC::LQR_K_calc(float *coe,float len)
{

  return coe[0]*len*len*len+coe[1]*len*len+coe[2]*len+coe[3];
}



/* 变量刷新 */
void VMC::Reset()
{

	vmc_leg.L0=0;
    vmc_leg.phi0=0;
	vmc_leg.alpha=0;
    vmc_leg.d_alpha=0;

	vmc_leg.lBD=0;

	vmc_leg.d_phi0=0;
	vmc_leg.last_phi0=0;

	vmc_leg.A0=0;
    vmc_leg.B0=0;
    vmc_leg.C0=0;
	vmc_leg.phi2=0;
    vmc_leg.phi3=0;

	vmc_leg.j11=0;
    vmc_leg.j12=0;
    vmc_leg.j21=0;
    vmc_leg.j22=0;
	vmc_leg.torque_set[0]=0;
    vmc_leg.torque_set[1]=0;

	vmc_leg.theta=0;
	vmc_leg.d_theta=0;
    vmc_leg.last_d_theta=0;
    vmc_leg.dd_theta=0;

	vmc_leg.d_L0=0;
	vmc_leg.dd_L0=0;
    vmc_leg.last_L0=0;
    vmc_leg.last_d_L0=0;
    vmc_leg.first_flag=0;
	vmc_leg.leg_flag=0;
}
