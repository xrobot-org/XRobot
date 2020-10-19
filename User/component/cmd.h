/*
  控制命令
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "ahrs.h"

typedef enum {
  CHASSIS_MODE_RELAX, /* 放松模式，电机不输出。一般情况底盘初始化之后的模式 */
  CHASSIS_MODE_BREAK, /* 刹车模式，电机闭环控制保持静止。用于机器人停止状态 */
  CHASSIS_MODE_FOLLOW_GIMBAL, /* 通过闭环控制使车头方向跟随云台 */
  CHASSIS_MODE_ROTOR, /* 小陀螺模式，通过闭环控制使底盘不停旋转 */
  CHASSIS_MODE_INDENPENDENT, /* 独立模式。底盘运行不受云台影响 */
  CHASSIS_MODE_OPEN, /* 开环模式。底盘运行不受PID控制，直接输出到电机 */
} CMD_ChassisMode_t; /* 底盘运行模式 */

typedef enum {
  GIMBAL_MODE_RELAX, /* 放松模式，电机不输出。一般情况云台初始化之后的模式 */
  GIMBAL_MODE_ABSOLUTE, /* 绝对坐标系控制，控制在空间内的绝对姿态 */
  GIMBAL_MODE_RELATIVE, /* 相对坐标系控制，控制相对于底盘的姿态 */
  GIMBAL_MODE_FIX, /* 固定模式。将云台固定于相对于底盘的正前方位置 */
} CMD_GimbalMode_t; /* 云台运行模式 */

typedef enum {
  SHOOT_MODE_RELAX, /* 放松模式，电机不输出。一般情况射击初始化之后的模式 */
  SHOOT_MODE_SAFE, /* 保险模式，电机闭环控制保持静止。保证安全状态 */
  SHOOT_MODE_STDBY, /* 准备模式，摩擦轮开启。拨弹电机闭环控制保持静止 */
  SHOOT_MODE_FIRE, /* 开火模式，摩擦轮开启。拨弹电机开启 */
} CMD_ShootMode_t; /* 射击运行模式 */

typedef struct {
  CMD_ChassisMode_t mode; /* 底盘运行模式 */
  MoveVector_t ctrl_vec;  /* 底盘控制向量 */
} CMD_ChassisCmd_t;       /* 底盘控制命令 */

typedef struct {
  CMD_GimbalMode_t mode;  /* 云台运行模式 */
  AHRS_Eulr_t delta_eulr; /* 欧拉角变化角度 */
} CMD_GimbalCmd_t;        /* 云台控制命令 */

typedef struct {
  CMD_ShootMode_t mode; /* 射击运行模式 */
  float bullet_speed;   /* 子弹初速 */
  float shoot_freq_hz;  /* 射击频率 */
} CMD_ShootCmd_t;       /* 射击控制命令 */

typedef enum {
  CMD_SW_ERR = 0,
  CMD_SW_UP = 1,
  CMD_SW_MID = 3,
  CMD_SW_DOWN = 2,
} CMD_SwitchPos_t; /* 拨杆位置 */

typedef enum {
  CMD_KEY_W = 0,
  CMD_KEY_S,
  CMD_KEY_A,
  CMD_KEY_D,
  CMD_KEY_Q,
  CMD_KEY_E,
  CMD_KEY_SHIFT,
  CMD_KEY_CTRL,
} CMD_KeyValue_t; /* 键盘按键值 */

typedef struct {
  float sens_mouse; /* 鼠标灵敏度 */
  float sens_rc;    /* 遥控器摇杆灵敏度 */
} CMD_Params_t;     /* 命令参数 */

typedef struct {
  bool pc_ctrl; /* 是否使用键鼠控制 */

  const CMD_Params_t *param;

  CMD_ChassisCmd_t chassis;
  CMD_GimbalCmd_t gimbal;
  CMD_ShootCmd_t shoot;
} CMD_t;

typedef struct {
  float ch_l_x; /* 遥控器左侧摇杆横轴值，上为正 */
  float ch_l_y; /* 遥控器左侧摇杆纵轴值，右为正 */
  float ch_r_x; /* 遥控器右侧摇杆横轴值，上为正 */
  float ch_r_y; /* 遥控器右侧摇杆纵轴值，右为正 */

  float ch_res; /* 第五通道值 */

  CMD_SwitchPos_t sw_r; /* 右侧拨杆位置 */
  CMD_SwitchPos_t sw_l; /* 左侧拨杆位置 */

  struct {
    int16_t x;
    int16_t y;
    int16_t z;
    bool l_click; /* 左键 */
    bool r_click; /* 右键 */
  } mouse;        /* 鼠标值 */

  uint16_t key; /* 按键值 */
  uint16_t res; /* 保留，未启用 */
} CMD_RC_t;

int8_t CMD_Init(CMD_t *cmd, const CMD_Params_t *param);
int8_t CMD_Parse(const CMD_RC_t *rc, CMD_t *cmd);

#ifdef __cplusplus
}
#endif
