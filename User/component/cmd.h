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

#define CMD_REFEREE_MAX_NUM (3) /* 发送命令限定的最大数量 */

/* 底盘运行模式 */
typedef enum {
  CHASSIS_MODE_RELAX, /* 放松模式，电机不输出。一般情况底盘初始化之后的模式 */
  CHASSIS_MODE_BREAK, /* 刹车模式，电机闭环控制保持静止。用于机器人停止状态 */
  CHASSIS_MODE_FOLLOW_GIMBAL, /* 通过闭环控制使车头方向跟随云台 */
  CHASSIS_MODE_FOLLOW_GIMBAL_35, /* 通过闭环控制使车头方向35度跟随云台 */
  CHASSIS_MODE_ROTOR, /* 小陀螺模式，通过闭环控制使底盘不停旋转 */
  CHASSIS_MODE_INDENPENDENT, /* 独立模式。底盘运行不受云台影响 */
  CHASSIS_MODE_OPEN, /* 开环模式。底盘运行不受PID控制，直接输出到电机 */
} CMD_ChassisMode_t;

/* 云台运行模式 */
typedef enum {
  GIMBAL_MODE_RELAX, /* 放松模式，电机不输出。一般情况云台初始化之后的模式 */
  GIMBAL_MODE_ABSOLUTE, /* 绝对坐标系控制，控制在空间内的绝对姿态 */
  GIMBAL_MODE_RELATIVE, /* 相对坐标系控制，控制相对于底盘的姿态 */
} CMD_GimbalMode_t;

/* 射击运行模式 */
typedef enum {
  SHOOT_MODE_RELAX,  /* 放松模式，电机不输出 */
  SHOOT_MODE_SAFE,   /* 保险模式，电机闭环控制保持静止 */
  SHOOT_MODE_LOADED, /* 上膛模式，摩擦轮开启。随时准备开火 */
} CMD_ShootMode_t;

typedef enum {
  FIRE_MODE_SINGLE, /* 单发开火模式  */
  FIRE_MODE_BURST,  /* N连发开火模式 */
  FIRE_MODE_CONT,   /* 持续开火模式 */
  FIRE_MODE_NUM,
} CMD_FireMode_t;

/* 小陀螺转动模式 */
typedef enum {
  ROTOR_MODE_CW,   /* 顺时针转动 */
  ROTOR_MODE_CCW,  /* 逆时针转动 */
  ROTOR_MODE_RAND, /* 随机转动 */
} CMD_RotorMode_t;

/* 底盘控制命令 */
typedef struct {
  CMD_ChassisMode_t mode;     /* 底盘运行模式 */
  CMD_RotorMode_t mode_rotor; /* 小陀螺转动模式 */
  MoveVector_t ctrl_vec;      /* 底盘控制向量 */
} CMD_ChassisCmd_t;

/* 云台控制命令 */
typedef struct {
  CMD_GimbalMode_t mode;  /* 云台运行模式 */
  AHRS_Eulr_t delta_eulr; /* 欧拉角变化角度 */
} CMD_GimbalCmd_t;

/* 射击控制命令 */
typedef struct {
  CMD_ShootMode_t mode;     /* 射击运行模式 */
  CMD_FireMode_t fire_mode; /* 开火模式 */
  bool fire;                /*开火*/
  bool cover_open;          /* 弹舱盖开关 */
  bool reverse_trig;        /* 拨弹电机状态 */
} CMD_ShootCmd_t;

/* 拨杆位置 */
typedef enum {
  CMD_SW_ERR = 0,
  CMD_SW_UP = 1,
  CMD_SW_MID = 3,
  CMD_SW_DOWN = 2,
} CMD_SwitchPos_t;

/* 键盘按键值 */
typedef enum {
  CMD_KEY_W = 0,
  CMD_KEY_S,
  CMD_KEY_A,
  CMD_KEY_D,
  CMD_KEY_SHIFT,
  CMD_KEY_CTRL,
  CMD_KEY_Q,
  CMD_KEY_E,
  CMD_KEY_R,
  CMD_KEY_F,
  CMD_KEY_G,
  CMD_KEY_Z,
  CMD_KEY_X,
  CMD_KEY_C,
  CMD_KEY_V,
  CMD_KEY_B,
  CMD_L_CLICK,
  CMD_R_CLICK,
  CMD_KEY_NUM,
} CMD_KeyValue_t;

/* 行为值序列 */
typedef enum {
  CMD_BEHAVIOR_FORE = 0,       /* 向前 */
  CMD_BEHAVIOR_BACK,           /* 向后 */
  CMD_BEHAVIOR_LEFT,           /* 向左 */
  CMD_BEHAVIOR_RIGHT,          /* 向右 */
  CMD_BEHAVIOR_ACCELERATE,     /* 加速 */
  CMD_BEHAVIOR_DECELEBRATE,    /* 减速 */
  CMD_BEHAVIOR_FIRE,           /* 开火 */
  CMD_BEHAVIOR_FIRE_MODE,      /* 切换开火模式 */
  CMD_BEHAVIOR_BUFF,           /* 打符模式 */
  CMD_BEHAVIOR_AUTOAIM,        /* 自瞄模式 */
  CMD_BEHAVIOR_OPENCOVER,      /* 弹舱盖开关 */
  CMD_BEHAVIOR_ROTOR,          /* 小陀螺模式 */
  CMD_BEHAVIOR_REVTRIG,        /* 反转拨弹 */
  CMD_BEHAVIOR_FOLLOWGIMBAL35, /* 跟随云台呈35度 */
  CMD_BEHAVIOR_NUM,
} CMD_Behavior_t;

typedef enum {
  CMD_ACTIVE_PRESSING, /* 按下时触发 */
  CMD_ACTIVE_RASING,   /* 抬起时触发 */
  CMD_ACTIVE_PRESSED,  /* 按住时触发 */
} CMD_ActiveType_t;

typedef struct {
  CMD_ActiveType_t active;
  CMD_KeyValue_t key;
} CMD_KeyMapItem_t;

/* 行为映射的对应按键数组 */
typedef struct {
  CMD_KeyMapItem_t key_map[CMD_BEHAVIOR_NUM];
} CMD_KeyMap_Params_t;

/* 位移灵敏度参数 */
typedef struct {
  float move_norm_sense; /* 移动灵敏度 */
  float move_fast_sense; /* 加速灵敏度 */
  float move_slow_sense; /* 减速灵敏度 */
} CMD_MoveParams_t;

typedef struct {
  uint16_t width;
  uint16_t height;
} CMD_Screen_t;

/* 命令参数 */
typedef struct {
  float sens_mouse;        /* 鼠标灵敏度 */
  float sens_stick;        /* 遥控器摇杆灵敏度 */
  CMD_KeyMap_Params_t map; /* 按键映射行为命令 */
  CMD_MoveParams_t move;   /* 位移灵敏度参数 */
  CMD_Screen_t screen;     /* 屏幕分辨率参数 */
} CMD_Params_t;

/* AI行为状态 */
typedef enum {
  AI_STATUS_STOP,      /* 停止状态 */
  AI_STATUS_AUTOAIM,   /* 自瞄状态 */
  AI_STATUS_HITSWITCH, /* 打符状态 */
  AI_STATUS_AUTOMATIC  /* 自动状态 */
} CMD_AI_Status_t;

/* UI所用行为状态 */
typedef enum {
  CMD_UI_NOTHING,          /* 当前无状态 */
  CMD_UI_AUTO_AIM_START,   /* 自瞄状态开启 */
  CMD_UI_AUTO_AIM_STOP,    /* 自瞄状态关闭 */
  CMD_UI_HIT_SWITCH_START, /* 打符状态开启 */
  CMD_UI_HIT_SWITCH_STOP   /* 打符状态关闭 */
} CMD_UI_t;

/*裁判系统发送的命令*/
typedef struct {
  CMD_UI_t cmd[CMD_REFEREE_MAX_NUM]; /* 命令数组 */
  uint8_t counter;                   /* 命令计数 */
} CMD_RefereeCmd_t;

typedef struct {
  int16_t x;
  int16_t y;
  int16_t z;

  /* 键位 */
  struct {
    bool l; /* 左键 */
    bool r; /* 右键 */
  } click;
} CMD_Mouse_t; /* 鼠标值 */

typedef struct {
  bool pc_ctrl;        /* 是否使用键鼠控制 */
  bool host_overwrite; /* 是否Host控制 */
  uint16_t key_last;   /* 上次按键键值 */

  CMD_Mouse_t mouse_last; /* 鼠标值 */

  CMD_AI_Status_t ai_status; /* AI状态 */

  const CMD_Params_t *param; /* 命令参数 */

  CMD_ChassisCmd_t chassis; /* 底盘控制命令 */
  CMD_GimbalCmd_t gimbal;   /* 云台控制命令 */
  CMD_ShootCmd_t shoot;     /* 射击控制命令 */
  CMD_RefereeCmd_t referee; /* 裁判系统发送命令 */
} CMD_t;

typedef struct {
  float ch_l_x; /* 遥控器左侧摇杆横轴值，上为正 */
  float ch_l_y; /* 遥控器左侧摇杆纵轴值，右为正 */
  float ch_r_x; /* 遥控器右侧摇杆横轴值，上为正 */
  float ch_r_y; /* 遥控器右侧摇杆纵轴值，右为正 */

  float ch_res; /* 第五通道值 */

  CMD_SwitchPos_t sw_r; /* 右侧拨杆位置 */
  CMD_SwitchPos_t sw_l; /* 左侧拨杆位置 */

  CMD_Mouse_t mouse; /* 鼠标值 */

  uint16_t key; /* 按键值 */

  uint16_t res; /* 保留，未启用 */
} CMD_RC_t;

typedef struct {
  AHRS_Eulr_t gimbal_delta; /* 欧拉角的变化量 */

  struct {
    float vx;         /* x轴移动速度 */
    float vy;         /* y轴移动速度 */
    float wz;         /* z轴转动速度 */
  } chassis_move_vec; /* 底盘移动向量 */

  bool fire; /* 开火状态 */
} CMD_Host_t;

/**
 * @brief 解析行为命令
 *
 * @param rc 遥控器数据
 * @param cmd 主结构体
 */
int8_t CMD_Init(CMD_t *cmd, const CMD_Params_t *param);

/**
 * @brief 检查是否启用上位机控制指令覆盖
 *
 * @param cmd 主结构体
 * @return true 启用
 * @return false 不启用
 */
bool CMD_CheckHostOverwrite(CMD_t *cmd);

/**
 * @brief 解析命令
 *
 * @param rc 遥控器数据
 * @param cmd 命令
 * @param dt_sec 两次解析的间隔
 * @return int8_t 0对应没有错误
 */
int8_t CMD_ParseRc(CMD_RC_t *rc, CMD_t *cmd, float dt_sec);

/**
 * @brief 解析上位机命令
 *
 * @param host host数据
 * @param cmd 命令
 * @param dt_sec 两次解析的间隔
 * @return int8_t 0对应没有错误
 */
int8_t CMD_ParseHost(const CMD_Host_t *host, CMD_t *cmd, float dt_sec);

/**
 * @brief 添加向Referee发送的命令
 *
 * @param ref 命令队列
 * @param cmd 要添加的命令
 * @return int8_t 0对应没有错误
 */
int8_t CMD_RefereeAdd(CMD_RefereeCmd_t *ref, CMD_UI_t cmd);

#ifdef __cplusplus
}
#endif
