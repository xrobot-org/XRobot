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
/* Exported functions prototypes -------------------------------------------- */

/**
 * @brief 运行电容控制逻辑
 *
 * @param cap 电容数据结构体
 * @param referee 裁判系统数据
 * @param cap_out 电容输出结构体
 */
void Cap_Control(CAN_Capacitor_t *cap, const Referee_ForCap_t *referee,
                 CAN_CapOutput_t *cap_out);

/**
 * @brief 电容模块离线时的控制逻辑
 *
 * @param cap 电容数据结构体
 * @param cap_out 电容输出结构体
 * @param power_chassis 底盘功率
 */
void Cap_HandleOffline(CAN_Capacitor_t *cap, CAN_CapOutput_t *cap_out,
                       float power_chassis);

/**
 * @brief 导出电容数据
 *
 * @param cap 电容数据
 * @param ui 结构体
 */
void Cap_DumpUI(const CAN_Capacitor_t *cap, Referee_CapUI_t *ui);
