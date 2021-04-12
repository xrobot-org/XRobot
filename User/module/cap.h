/*
 * 电容模组
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

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
 * @brief 导出电容数据
 *
 * @param cap 电容数据
 * @param ui 结构体
 */
void Cap_DumpUI(const CAN_Capacitor_t *cap, Referee_CapUI_t *ui);

#ifdef __cplusplus
}
#endif
