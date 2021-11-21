/*
  自定义的类型
*/

#pragma once

/* 欧拉角（Euler angle） */
typedef struct {
  float yaw; /* 偏航角（Yaw angle） */
  float pit; /* 俯仰角（Pitch angle） */
  float rol; /* 翻滚角（Roll angle） */
} eulr_t;

/* 四元数 */
typedef struct {
  float q0;
  float q1;
  float q2;
  float q3;
} quaternion_t;

/* 移动向量 */
typedef struct {
  float vx; /* 前后平移 */
  float vy; /* 左右平移 */
  float wz; /* 转动 */
} move_vector_t;

/* 二元素向量 */
typedef struct {
  float x;
  float y;
} vector2_t;

/* 三元素向量 */
typedef struct {
  float x;
  float y;
  float z;
} vector3_t;

typedef enum {
  RM_OK = 0,
  ERR_FAIL,    /* General */
  ERR_PARAM,   /* Invalid parameter */
  ERR_NOPERM,  /* Operation not permitted */
  ERR_2BIG,    /* Argument list too long */
  ERR_NOMEM,   /* Out of memory */
  ERR_NOACCES, /* Permission denied */
  ERR_FAULT,   /* Bad address */
  ERR_NULL,    /* NULL pointer */
  ERR_NODEV,   /* No such device */
  ERR_TIMEOUT, /* Waited to long */
} err_t;

typedef enum {
  COLOR_HEX_WHITE = 0XFFFFFF,
  COLOR_HEX_SILVER = 0XC0C0C0,
  COLOR_HEX_GRAY = 0X808080,
  COLOR_HEX_BLACK = 0X000000,
  COLOR_HEX_RED = 0XFF0000,
  COLOR_HEX_MAROON = 0X800000,
  COLOR_HEX_YELLOW = 0XFFFF00,
  COLOR_HEX_OLIVE = 0X808000,
  COLOR_HEX_LIME = 0X00FF00,
  COLOR_HEX_GREEN = 0X008000,
  COLOR_HEX_AQUA = 0X00FFFF,
  COLOR_HEX_TEAL = 0X008080,
  COLOR_HEX_BLUE = 0X0000FF,
  COLOR_HEX_NAVY = 0X000080,
  COLOR_HEX_FUCHSIA = 0XFF00FF,
  COLOR_HEX_PURPLE = 0X800080,
} color_hex_t;
