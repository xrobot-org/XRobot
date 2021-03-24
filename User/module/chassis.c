/*
 * 底盘模组
 */

/* Includes ----------------------------------------------------------------- */
#include "chassis.h"

#include "bsp\mm.h"
#include "component\limiter.h"
#include "device\can.h"
#include "module\cap.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
#define _CAP_PERCENTAGE_WORK 80   /* 底盘不再限制功率的电容电量 */
#define _CAP_PERCENTAGE_CHARGE 30 /* 电容开始工作的电容电量 */

#define CHASSIS_MAX_CAP_POWER 100 /* 电容能够提供的最大功率 */
#define CHASSIS_ROTOR_VEC_WZ 0.5f /* 小陀螺旋转位移 */
#define CHASSIS_ROTOR_ROTATE_ANGLE (M_PI / 2.0f) /* 小陀螺旋转弧度 */

/* Private macro ------------------------------------------------------------ */

/* 保证电容电量宏定义在正确范围内 */
#if ((_CAP_PERCENTAGE_WORK < 0) || (_CAP_PERCENTAGE_WORK > 100) || \
     (_CAP_PERCENTAGE_CHARGE < 0) || (_CAP_PERCENTAGE_CHARGE > 100))
#error "Cap percentage should be in the range from 0 to 100."
#endif

/* 保证电容功率宏定义在正确范围内 */
#if ((CHASSIS_MAX_CAP_POWER < 60) || (CHASSIS_MAX_CAP_POWER > 200))
#error "The capacitor power should be in in the range from 60 to 200."
#endif

/* Private variables -------------------------------------------------------- */

static const float CAP_PERCENTAGE_WORK = (float)_CAP_PERCENTAGE_WORK / 100.0f;
static const float CAP_PERCENTAGE_CHARGE =
    (float)_CAP_PERCENTAGE_CHARGE / 100.0f;

/* Private function  -------------------------------------------------------- */

/**
 * \brief 设置底盘模式
 *
 * \param c 包含底盘数据的结构体
 * \param mode 要设置的模式
 *
 * \return 函数运行结果
 */
static int8_t Chassis_SetMode(Chassis_t *c, CMD_ChassisMode_t mode) {
  if (c == NULL) return CHASSIS_ERR_NULL; /* 主结构体不能为空 */
  if (mode == c->mode) return CHASSIS_OK; /* 模式未改变直接返回 */

  c->mode = mode;

  /* 切换模式后重置PID和滤波器 */
  for (uint8_t i = 0; i < c->num_wheel; i++) {
    PID_Reset(c->pid.motor + i);
    LowPassFilter2p_Reset(c->filter.in + i, 0.0f);
    LowPassFilter2p_Reset(c->filter.out + i, 0.0f);
  }

  return CHASSIS_OK;
}

/* Exported functions ------------------------------------------------------- */

/**
 * \brief 初始化底盘
 *
 * \param c 包含底盘数据的结构体
 * \param param 包含底盘参数的结构体指针
 * \param target_freq 任务预期的运行频率
 *
 * \return 函数运行结果
 */
int8_t Chassis_Init(Chassis_t *c, const Chassis_Params_t *param,
                    AHRS_Eulr_t *mech_zero, float target_freq) {
  if (c == NULL) return CHASSIS_ERR_NULL;

  c->param = param;             /* 初始化参数 */
  c->mode = CHASSIS_MODE_RELAX; /* 设置上电后底盘默认模式 */
  c->mech_zero = mech_zero;     /* 设置底盘机械零点 */

  /* 如果电机反装重新计算机械零点 */
  if (param->reverse.yaw) {
    c->mech_zero->yaw = -(c->mech_zero->yaw) + M_2PI;
  }

  /* 根据参数（param）中的底盘型号初始化Mixer */
  Mixer_Mode_t mixer_mode;
  switch (c->param->type) {
    case CHASSIS_TYPE_MECANUM:
      c->num_wheel = 4;
      mixer_mode = MIXER_MECANUM;
      break;

    case CHASSIS_TYPE_PARLFIX4:
      c->num_wheel = 4;
      mixer_mode = MIXER_PARLFIX4;
      break;

    case CHASSIS_TYPE_PARLFIX2:
      c->num_wheel = 2;
      mixer_mode = MIXER_PARLFIX2;
      break;

    case CHASSIS_TYPE_OMNI_CROSS:
      c->num_wheel = 4;
      mixer_mode = MIXER_OMNICROSS;
      break;

    case CHASSIS_TYPE_OMNI_PLUS:
      c->num_wheel = 4;
      mixer_mode = MIXER_OMNIPLUS;
      break;

    case CHASSIS_TYPE_SINGLE:
      c->num_wheel = 1;
      mixer_mode = MIXER_SINGLE;
      break;

    case CHASSIS_TYPE_DRONE:
      /* onboard sdk. */
      return CHASSIS_ERR_TYPE;
  }

  /* 根据底盘型号动态分配控制时使用的变量 */
  c->feedback.motor_rpm =
      BSP_Malloc((size_t)c->num_wheel * sizeof(*c->feedback.motor_rpm));
  if (c->feedback.motor_rpm == NULL) goto error; /* 变量未分配，返回错误 */

  c->feedback.motor_current =
      BSP_Malloc((size_t)c->num_wheel * sizeof(*c->feedback.motor_current));
  if (c->feedback.motor_current == NULL) goto error;

  c->setpoint.motor_rpm =
      BSP_Malloc((size_t)c->num_wheel * sizeof(*c->setpoint.motor_rpm));
  if (c->setpoint.motor_rpm == NULL) goto error;

  c->pid.motor = BSP_Malloc((size_t)c->num_wheel * sizeof(*c->pid.motor));
  if (c->pid.motor == NULL) goto error;

  c->out = BSP_Malloc((size_t)c->num_wheel * sizeof(*c->out));
  if (c->out == NULL) goto error;

  c->filter.in = BSP_Malloc((size_t)c->num_wheel * sizeof(*c->filter.in));
  if (c->filter.in == NULL) goto error;

  c->filter.out = BSP_Malloc((size_t)c->num_wheel * sizeof(*c->filter.out));
  if (c->filter.out == NULL) goto error;

  /* 初始化轮子电机控制PID和LPF */
  for (uint8_t i = 0; i < c->num_wheel; i++) {
    PID_Init(c->pid.motor + i, KPID_MODE_NO_D, target_freq,
             &(c->param->motor_pid_param));

    LowPassFilter2p_Init(c->filter.in + i, target_freq,
                         c->param->low_pass_cutoff_freq.in);
    LowPassFilter2p_Init(c->filter.out + i, target_freq,
                         c->param->low_pass_cutoff_freq.out);
  }

  /* 初始化跟随云台的控制PID */
  PID_Init(&(c->pid.follow), KPID_MODE_NO_D, target_freq,
           &(c->param->follow_pid_param));

  Mixer_Init(&(c->mixer), mixer_mode); /* 初始化混合器 */
  return CHASSIS_OK;

error:
  /* 动态内存分配错误时，释放已经分配的内存，返回错误值 */
  BSP_Free(c->feedback.motor_rpm);
  BSP_Free(c->setpoint.motor_rpm);
  BSP_Free(c->pid.motor);
  BSP_Free(c->out);
  BSP_Free(c->filter.in);
  BSP_Free(c->filter.out);
  return CHASSIS_ERR_NULL;
}

/**
 * \brief 更新底盘的反馈信息
 *
 * \param c 包含底盘数据的结构体
 * \param can CAN设备结构体
 *
 * \return 函数运行结果
 */
int8_t Chassis_UpdateFeedback(Chassis_t *c, const CAN_t *can) {
  /* 底盘数据和CAN结构体不能为空 */
  if (c == NULL) return CHASSIS_ERR_NULL;
  if (can == NULL) return CHASSIS_ERR_NULL;

  /* 如果电机反装重新计算正确的反馈值 */
  if (c->param->reverse.yaw) {
    c->feedback.gimbal_yaw_angle =
        -can->motor.gimbal.named.yaw.rotor_angle + M_2PI;
  } else {
    c->feedback.gimbal_yaw_angle = can->motor.gimbal.named.yaw.rotor_angle;
  }

  /* 将CAN中的反馈数据写入到feedback中 */
  for (uint8_t i = 0; i < c->num_wheel; i++) {
    c->feedback.motor_rpm[i] = can->motor.chassis.as_array[i].rotor_speed;
    c->feedback.motor_current[i] =
        can->motor.chassis.as_array[i].torque_current;
  }

  return CHASSIS_OK;
}

/**
 * \brief 运行底盘控制逻辑
 *
 * \param c 包含底盘数据的结构体
 * \param c_cmd 底盘控制指令
 * \param dt_sec 两次调用的时间间隔
 *
 * \return 函数运行结果
 */
int8_t Chassis_Control(Chassis_t *c, const CMD_ChassisCmd_t *c_cmd,
                       float dt_sec) {
  /* 底盘数据和控制指令结构体不能为空 */
  if (c == NULL) return CHASSIS_ERR_NULL;
  if (c_cmd == NULL) return CHASSIS_ERR_NULL;
  /* 根据遥控器命令更改底盘模式 */
  Chassis_SetMode(c, c_cmd->mode);
  /* ctrl_vec -> move_vec 控制向量和真实的移动向量之间有一个换算关系 */
  /* 先计算vx、vy. */
  switch (c->mode) {
    case CHASSIS_MODE_BREAK: /* 刹车模式电机停止 */
      c->move_vec.vx = 0.0f;
      c->move_vec.vy = 0.0f;
      break;

    case CHASSIS_MODE_INDENPENDENT: /* 独立模式控制向量与运动向量相等 */
      c->move_vec.vx = c_cmd->ctrl_vec.vx;
      c->move_vec.vy = c_cmd->ctrl_vec.vx;
      break;

    case CHASSIS_MODE_OPEN:
    case CHASSIS_MODE_RELAX:
    case CHASSIS_MODE_FOLLOW_GIMBAL: /* 按照云台方向换算运动向量 */
    case CHASSIS_MODE_ROTOR: {
      float beta = c->feedback.gimbal_yaw_angle - c->mech_zero->yaw;
      float cos_beta = cosf(beta);
      float sin_beta = sinf(beta);
      c->move_vec.vx =
          cos_beta * c_cmd->ctrl_vec.vx - sin_beta * c_cmd->ctrl_vec.vy;
      c->move_vec.vy =
          sin_beta * c_cmd->ctrl_vec.vx + cos_beta * c_cmd->ctrl_vec.vy;
    }
  }

  /* 计算wz. */
  switch (c->mode) {
    case CHASSIS_MODE_RELAX:
    case CHASSIS_MODE_BREAK:
    case CHASSIS_MODE_INDENPENDENT: /* 独立模式wz为0 */
      c->move_vec.wz = 0.0f;
      break;

    case CHASSIS_MODE_OPEN:
    case CHASSIS_MODE_FOLLOW_GIMBAL: /* 跟随模式通过PID控制使车头跟随云台 */
      c->move_vec.wz = PID_Calc(
          &(c->pid.follow), 0, c->feedback.gimbal_yaw_angle - c->mech_zero->yaw,
          0.0f, dt_sec);
      break;
    case CHASSIS_MODE_ROTOR: { /* 小陀螺模式使底盘以一定速度旋转 */
      float beta = c->feedback.gimbal_yaw_angle - c->mech_zero->yaw;
      /* 根据转动模式计算wz */
      switch (c_cmd->mode_rotor) {
        case ROTOR_MODE_NONE:
          break;
        case ROTOR_MODE_CW:
          c->move_vec.wz = CHASSIS_ROTOR_VEC_WZ;
          break;
        case ROTOR_MODE_CCW:
          c->move_vec.wz = -CHASSIS_ROTOR_VEC_WZ;
          break;
        case ROTOR_MODE_NUM:
        case ROTOR_MODE_BOTH: {
          /* 小陀螺在指定的弧度范围内转动 */
          if (beta >= c->beta_last) {
            if (beta <= CHASSIS_ROTOR_ROTATE_ANGLE)
              c->move_vec.wz = CHASSIS_ROTOR_VEC_WZ;
            else
              c->move_vec.wz = -CHASSIS_ROTOR_VEC_WZ;
          } else {
            if (beta >= -CHASSIS_ROTOR_ROTATE_ANGLE)
              c->move_vec.wz = -CHASSIS_ROTOR_VEC_WZ;
            else
              c->move_vec.wz = CHASSIS_ROTOR_VEC_WZ;
          }
          c->beta_last = beta;
          break;
        }
      }
    }
  }

  /* move_vec -> motor_rpm_set. 通过运动向量计算轮子转速目标值 */
  Mixer_Apply(&(c->mixer), &(c->move_vec), c->setpoint.motor_rpm, c->num_wheel,
              9000.0f);

  /* 根据轮子转速目标值，利用PID计算电机输出值 */
  for (uint8_t i = 0; i < c->num_wheel; i++) {
    /* 输入滤波. */
    c->feedback.motor_rpm[i] =
        LowPassFilter2p_Apply(c->filter.in + i, c->feedback.motor_rpm[i]);

    /* 根据底盘模式计算输出值 */
    switch (c->mode) {
      case CHASSIS_MODE_BREAK:
      case CHASSIS_MODE_FOLLOW_GIMBAL:
      case CHASSIS_MODE_ROTOR:
      case CHASSIS_MODE_INDENPENDENT: /* 独立模式,受PID控制 */
        c->out[i] = PID_Calc(c->pid.motor + i, c->setpoint.motor_rpm[i],
                             c->feedback.motor_rpm[i], 0.0f, dt_sec);
        break;

      case CHASSIS_MODE_OPEN: /* 开环模式,不受PID控制 */
        c->out[i] = c->setpoint.motor_rpm[i] / 9000.0f;
        break;

      case CHASSIS_MODE_RELAX: /* 放松模式,不输出 */
        c->out[i] = 0;
        break;
    }
    /* 输出滤波. */
    c->out[i] = LowPassFilter2p_Apply(c->filter.out + i, c->out[i]);
  }

  return CHASSIS_OK;
}

/**
 * @brief 底盘功率限制
 *
 * @param c 底盘数据
 * @param cap 电容数据
 * @param ref 裁判系统数据
 * @return 函数运行结果
 */
int8_t Chassis_PowerLimit(Chassis_t *c, const CAN_Capacitor_t *cap,
                          const Referee_ForChassis_t *ref) {
  float power_limit = 0.0f;
  if (ref->ref_status != REF_STATUS_RUNNING) {
    /* 裁判系统离线，将功率限制为固定值 */
    power_limit = CHASSIS_POWER_MAX_WITHOUT_REF;
  } else {
    if (cap->cap_status == CAN_CAP_STATUS_RUNNING &&
        cap->percentage > CAP_PERCENTAGE_CHARGE) {
      /* 电容在线且电量足够，使用电容 */
      if (cap->percentage > CAP_PERCENTAGE_WORK) {
        /* 电容接近充满时不再限制功率 */
        power_limit = -1.0f;
      } else {
        /* 按照电容能量百分比计算输出功率 */
        power_limit = ref->chassis_power_limit +
                      (cap->percentage - CAP_PERCENTAGE_CHARGE) /
                          (CAP_PERCENTAGE_WORK - CAP_PERCENTAGE_CHARGE) *
                          (float)CHASSIS_MAX_CAP_POWER;
      }
    } else {
      /* 电容不在工作，根据缓冲能量计算输出功率限制 */
      power_limit = PowerLimit_TargetPower(ref->chassis_power_limit,
                                           ref->chassis_pwr_buff);
    }
  }
  /* 应用功率限制 */
  PowerLimit_ChassicOutput(power_limit, c->out, c->feedback.motor_rpm,
                           c->num_wheel);

  return CHASSIS_OK;
}

/**
 * \brief 复制底盘输出值
 *
 * \param s 包含底盘数据的结构体
 * \param out CAN设备底盘输出结构体
 */
void Chassis_DumpOutput(Chassis_t *c, CAN_ChassisOutput_t *out) {
  for (uint8_t i = 0; i < c->num_wheel; i++) {
    out->as_array[i] = c->out[i];
  }
}

/**
 * \brief 清空Chassis输出数据
 *
 * \param out CAN设备底盘输出结构体
 */
void Chassis_ResetOutput(CAN_ChassisOutput_t *out) {
  for (uint8_t i = 0; i < 4; i++) {
    out->as_array[i] = 0.0f;
  }
}
