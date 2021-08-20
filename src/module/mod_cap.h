/**
 * @file cap.h
 * @author cndabai
 * @brief 电容模组
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 */

#pragma once

/* Includes ----------------------------------------------------------------- */
#include "device/can.h"
#include "device/referee.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */

typedef struct {
  float percentage;
  CAN_CapStatus_t cap_status;
} Cap_t;

/* Exported functions prototypes -------------------------------------------- */

/**
 * @brief 更新电容数据
 *
 * @param cap 电容数据结构体
 * @param cap_fb CAN总线电容反馈数据
 */
void Cap_Update(Cap_t *cap, const CAN_CapFeedback_t *cap_fb);

/**
 * @brief 运行电容控制逻辑
 *
 * @param cap 电容数据结构体
 * @param referee 裁判系统数据
 * @param cap_out 电容输出结构体
 */
void Cap_Control(const Referee_ForCap_t *referee, CAN_CapOutput_t *cap_out);

/**
 * @brief 电容模块离线时的控制逻辑
 *
 * @param cap 电容数据结构体
 * @param cap_out 电容输出结构体
 * @param power_chassis 底盘功率
 */
void Cap_HandleOffline(Cap_t *cap, CAN_CapOutput_t *cap_out,
                       float power_chassis);

/**
 * @brief 导出电容数据
 *
 * @param cap 电容数据
 * @param ui 结构体
 */
void Cap_PackUi(const Cap_t *cap, UI_CapUI_t *ui);
