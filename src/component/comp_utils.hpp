/*
  自定义的小工具
*/

#pragma once

#include "comp_type.hpp"

/**
 * @brief 计算时间戳相差秒数
 *
 */
#define TIME_DIFF(_start, _end) ((float)(_end - _start) / 1000000.0f)
#define TIME_DIFF_US(_start, _end) ((float)(_end - _start) / 1000000.0f)
#define TIME_DIFF_MS(_start, _end) ((float)(_end - _start) / 1000.0f)

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

int float_to_uint(float x, float x_min, float x_max, int bits);

float uint_to_float(int x_int, float x_min, float x_max, int bits);
