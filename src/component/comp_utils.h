/*
  自定义的小工具
*/

#pragma once

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

//#include "stm32f4xx.h"

#define M_DEG2RAD_MULT (0.01745329251f)
#define M_RAD2DEG_MULT (57.2957795131f)

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#ifndef M_2PI
#define M_2PI 6.28318530717958647692f
#endif

#ifndef M_1G
#define M_1G 9.80665ff
#endif

/**
 * @brief 返回两个值中的最大值
 *
 */
#define MAX(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b;      \
  })

/**
 * @brief 返回两个值中的最小值
 *
 */
#define MIN(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b;      \
  })

#ifdef MCU_DEBUG_BUILD

/**
 * @brief 如果表达式的值为假则运行处理函数
 *
 */
#define ASSERT(expr)                    \
  do {                                  \
    if (!(expr)) {                      \
      VerifyFailed(__FILE__, __LINE__); \
    }                                   \
  } while (0)
#else

/**
 * @brief 未定DEBUG，表达式不会运行，断言被忽略
 *
 */
#define ASSERT(expr) ((void)(0))
#endif

#ifdef MCU_DEBUG_BUILD

/**
 * @brief 如果表达式的值为假则运行处理函数
 *
 */
#define VERIFY(expr)                    \
  do {                                  \
    if (!(expr)) {                      \
      VerifyFailed(__FILE__, __LINE__); \
    }                                   \
  } while (0)
#else

/**
 * @brief 表达式会运行，忽略表达式结果
 *
 */
#define VERIFY(expr) ((void)(expr))
#endif

#ifndef RM_UNUSED

/**
 * @brief 标记未使用的参数，防止编译器警告
 *
 */
#define RM_UNUSED(X) ((void)X)
#endif

/**
 * @brief 获取结构体或者联合成员的容器
 *
 */
#define CONTAINER_OF(ptr, type, member)                \
  ({                                                   \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); \
  })

/**
 * @brief 获取数组长度
 *
 */
#define ARRAY_LEN(array) (sizeof((array)) / sizeof(*(array)))

/* 移动向量 */
typedef struct {
  float vx; /* 前后平移 */
  float vy; /* 左右平移 */
  float wz; /* 转动 */
} MoveVector_t;

/* 二元素向量 */
typedef struct {
  float x;
  float y;
} Vector2_t;

/* 三元素向量 */
typedef struct {
  float x;
  float y;
  float z;
} Vector3_t;

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
} Err_t;

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
} ColorHex_t;

/**
 * @brief 计算平方根倒数
 *
 * @param x 输入
 * @return float 计算结果
 */
float InvSqrt(float x);

/**
 * @brief 将值限制在-limit和limit之间。
 *
 * @param x 输入
 * @param limit 上下界的绝对值
 * @return float 操作后的值
 */
float AbsClamp(float x, float limit);

/**
 * @brief 将值限制在下限和上限之间。
 *
 * @param origin 被操作的值
 * @param lo 下限
 * @param hi 上限
 */
void Clamp(float *origin, float lo, float hi);

/**
 * @brief 符号函数
 *
 * @param in 输入
 * @return float 运算结果
 */
float Sign(float x);

/**
 * @brief 将运动向量置零
 *
 * @param mv 被操作的值
 */
void ResetMoveVector(MoveVector_t *mv);

/**
 * @brief 计算循环值的误差，用于没有负数值，并在一定范围内变化的值
 * 例如编码器：相差1.5PI其实等于相差-0.5PI
 *
 * @param sp 被操作的值
 * @param fb 变化量
 * @param range 被操作的值变化范围，正数时起效
 *
 * @return 函数运行结果
 */
float CircleError(float sp, float fb, float range);

/**
 * @brief 循环加法，用于没有负数值，并在一定范围内变化的值
 * 例如编码器，在0-2PI内变化，1.5PI + 1.5PI = 1PI
 *
 * @param origin 被操作的值
 * @param delta 变化量
 * @param range 被操作的值变化范围，正数时起效
 */
void CircleAdd(float *origin, float delta, float range);

/**
 * @brief 循环值取反
 *
 * @param origin 被操作的值
 */
void CircleReverse(float *origin);

/**
 * @brief 根据目标弹丸速度计算摩擦轮转速
 *
 * @param bullet_speed 弹丸速度
 * @param fric_radius 摩擦轮半径
 * @param is17mm 是否为17mm
 * @return 摩擦轮转速
 */
float BulletSpeedToFricRpm(float bullet_speed, float fric_radius, bool is17mm);

/**
 * @brief 断言失败处理
 *
 * @param file 文件名
 * @param line 行号
 */
void VerifyFailed(const char *file, uint32_t line);
