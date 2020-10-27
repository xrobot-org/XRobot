/*
 * 底盘模组
 */

/* Includes ----------------------------------------------------------------- */
#include "chassis.h"

#include "bsp\mm.h"
#include "component\limiter.h"
#include "device\can.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
/* Private function  -------------------------------------------------------- */

/*!
 * \brief 设置底盘模式
 *
 * \param c 包含底盘数据的结构体
 * \param mode 要设置的模式
 *
 * \return 函数运行结果
 */
static int8_t Chassis_SetMode(Chassis_t *c, CMD_ChassisMode_t mode) {
  if (c == NULL) return CHASSIS_ERR_NULL;
  if (mode == c->mode) return CHASSIS_OK;

  c->mode = mode;

  /* 切换模式后重置PID和滤波器 */
  for (uint8_t i = 0; i < c->num_wheel; i++) {
    PID_Reset(c->pid.motor + i);
    LowPassFilter2p_Reset(c->filter.in + i, 0.0f);
    LowPassFilter2p_Reset(c->filter.out + i, 0.0f);
  }

  // TODO: Check mode switchable.
  switch (mode) {
    case CHASSIS_MODE_RELAX:
      break;

    case CHASSIS_MODE_BREAK:
      break;

    case CHASSIS_MODE_FOLLOW_GIMBAL:
      break;

    case CHASSIS_MODE_ROTOR:
      break;

    case CHASSIS_MODE_INDENPENDENT:
      break;

    case CHASSIS_MODE_OPEN:
      break;
  }
  return CHASSIS_OK;
}

/* Exported functions ------------------------------------------------------- */

/*!
 * \brief 初始化底盘
 *
 * \param c 包含底盘数据的结构体
 * \param param 包含底盘参数的结构体指针
 * \param target_freq 任务预期的运行频率
 *
 * \return 函数运行结果
 */
int8_t Chassis_Init(Chassis_t *c, const Chassis_Params_t *param,
                    float target_freq) {
  if (c == NULL) return CHASSIS_ERR_NULL;

  c->param = param;             /* 初始化参数 */
  c->mode = CHASSIS_MODE_RELAX; /* 设置默认模式 */

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

    case CHASSIS_TYPE_DRONE:
      // onboard sdk.
      return CHASSIS_ERR_TYPE;
  }

  /* 根据底盘型号动态分配控制时使用的变量 */
  c->feedback.motor_rpm =
      BSP_Malloc((size_t)c->num_wheel * sizeof(*c->feedback.motor_rpm));
  if (c->feedback.motor_rpm == NULL) goto error;

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

/*!
 * \brief 更新底盘的反馈信息
 *
 * \param c 包含底盘数据的结构体
 * \param can CAN设备结构体
 *
 * \return 函数运行结果
 */
int8_t Chassis_UpdateFeedback(Chassis_t *c, const CAN_t *can) {
  if (c == NULL) return CHASSIS_ERR_NULL;
  if (can == NULL) return CHASSIS_ERR_NULL;

  c->feedback.gimbal_yaw_angle =
      can->gimbal_motor_feedback[CAN_MOTOR_GIMBAL_YAW].rotor_angle;

  for (uint8_t i = 0; i < c->num_wheel; i++) {
    c->feedback.motor_rpm[i] = can->chassis_motor_feedback[i].rotor_speed;
  }

  return CHASSIS_OK;
}

/*!
 * \brief 运行底盘控制逻辑
 *
 * \param c 包含底盘数据的结构体
 * \param c_cmd 底盘控制指令
 * \param dt_sec 两次调用的时间间隔
 *
 * \return 函数运行结果
 */
int8_t Chassis_Control(Chassis_t *c, CMD_ChassisCmd_t *c_cmd, float dt_sec) {
  if (c == NULL) return CHASSIS_ERR_NULL;
  if (c_cmd == NULL) return CHASSIS_ERR_NULL;

  Chassis_SetMode(c, c_cmd->mode);

  /* ctrl_vec -> move_vec 控制向量和真实的移动向量之间有一个换算关系 */
  /* 先计算vx、vy. */
  switch (c->mode) {
    case CHASSIS_MODE_BREAK:
      c->move_vec.vx = 0.0f;
      c->move_vec.vy = 0.0f;
      break;

    case CHASSIS_MODE_INDENPENDENT:
      c->move_vec.vx = c_cmd->ctrl_vec.vx;
      c->move_vec.vy = c_cmd->ctrl_vec.vx;
      break;

    case CHASSIS_MODE_OPEN:
    case CHASSIS_MODE_RELAX:
    case CHASSIS_MODE_FOLLOW_GIMBAL:
    case CHASSIS_MODE_ROTOR: {
      const float cos_beta = cosf(c->feedback.gimbal_yaw_angle);
      const float sin_beta = sinf(c->feedback.gimbal_yaw_angle);
      c->move_vec.vx =
          cos_beta * c_cmd->ctrl_vec.vx - sin_beta * c_cmd->ctrl_vec.vy;
      c->move_vec.vy =
          sin_beta * c_cmd->ctrl_vec.vx - cos_beta * c_cmd->ctrl_vec.vy;
    }
  }

  /* 计算wz. */
  switch (c->mode) {
    case CHASSIS_MODE_RELAX:
    case CHASSIS_MODE_BREAK:
    case CHASSIS_MODE_INDENPENDENT:
      c->move_vec.wz = 0.0f;
      break;

    case CHASSIS_MODE_OPEN:
    case CHASSIS_MODE_FOLLOW_GIMBAL:
      c->move_vec.wz = PID_Calc(&(c->pid.follow), 0,
                                c->feedback.gimbal_yaw_angle, 0.0f, dt_sec);
      break;
    case CHASSIS_MODE_ROTOR:
      c->move_vec.wz = 0.5f;
  }

  /* move_vec -> motor_rpm_set. 通过运动向量计算轮子转速目标值 */
  Mixer_Apply(&(c->mixer), c->move_vec.vx, c->move_vec.vy, c->move_vec.wz,
              c->setpoint.motor_rpm, c->num_wheel, 9000.0f);

  /* 根据轮子转速目标值，利用PID计算电机输出值 */
  for (uint8_t i = 0; i < 4; i++) {
    /* 输入滤波. */
    c->feedback.motor_rpm[i] =
        LowPassFilter2p_Apply(c->filter.in + i, c->feedback.motor_rpm[i]);

    switch (c->mode) {
      case CHASSIS_MODE_BREAK:
      case CHASSIS_MODE_FOLLOW_GIMBAL:
      case CHASSIS_MODE_ROTOR:
      case CHASSIS_MODE_INDENPENDENT:
        c->out[i] = PID_Calc(c->pid.motor + i, c->setpoint.motor_rpm[i],
                             c->feedback.motor_rpm[i], 0.0f, dt_sec);
        break;

      case CHASSIS_MODE_OPEN:
        c->out[i] = c->setpoint.motor_rpm[i] / 9000.0f;
        break;

      case CHASSIS_MODE_RELAX:
        c->out[i] = 0;
        break;
    }
    /* 输出滤波. */
    c->out[i] = LowPassFilter2p_Apply(c->filter.out + i, c->out[i]);
  }
  return CHASSIS_OK;
}
