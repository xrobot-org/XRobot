/*
  自定义的小工具
*/

#pragma once

#include <cfloat>
#include <cmath>
#include <cstdint>

#include "comp_type.hpp"

/**
 * @brief 角度到弧度转换
 *
 */
#define ANGLE2RANDIAN(_angle) (_angle / 360.0f * M_2PI)

/**
 * @brief 角速度（度每秒）转单位时间变化量（弧度）
 *
 */
#define SPEED2DELTA(_speed, _dt) (ANGLE2RANDIAN(_speed * _dt))

#ifndef MAX
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

#endif

#ifndef MIN
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

#endif

#ifdef MCU_DEBUG_BUILD

/**
 * @brief 如果表达式的值为假则运行处理函数
 *
 */
#define ASSERT(expr)                     \
  do {                                   \
    if (!(expr)) {                       \
      verify_failed(__FILE__, __LINE__); \
    }                                    \
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
#define VERIFY(expr)                     \
  do {                                   \
    if (!(expr)) {                       \
      verify_failed(__FILE__, __LINE__); \
    }                                    \
  } while (0)
#else

/**
 * @brief 表达式会运行，忽略表达式结果
 *
 */
#define VERIFY(expr) ((void)(expr))
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

/**
 * @brief 计算平方根倒数
 *
 * @param x 输入
 * @return float 计算结果
 */
float inv_sqrtf(float x);

/**
 * @brief 将值限制在-limit和limit之间。
 *
 * @param x 输入
 * @param limit 上下界的绝对值
 * @return float 操作后的值
 */
float abs_clampf(float x, float limit);

/**
 * @brief 将值限制在下限和上限之间。
 *
 * @param origin 被操作的值
 * @param lo 下限
 * @param hi 上限
 */
void clampf(float *origin, float lo, float hi);

/**
 * @brief 符号函数
 *
 * @param in 输入
 * @return float 运算结果
 */
float signf(float x);

/**
 * @brief 根据目标弹丸速度计算摩擦轮转速
 *
 * @param bullet_speed 弹丸速度
 * @param fric_radius 摩擦轮半径
 * @param is17mm 是否为17mm
 * @return 摩擦轮转速
 */
float bullet_speed_to_fric_rpm(float bullet_speed, float fric_radius,
                               bool is17mm);

bool gyro_is_stable(Component::Type::Vector3 *gyro);

/**
 * @brief 断言失败处理
 *
 * @param file 文件名
 * @param line 行号
 */
void verify_failed(const char *file, uint32_t line);

int float_to_uint(float x, float x_min, float x_max, int bits);

float uint_to_float(int x_int, float x_min, float x_max, int bits);
