/*
 * 电容模组
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------- */
#include "device\can.h"
#include "device\referee.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
/* Exported functions prototypes -------------------------------------------- */

void Cap_Control(CAN_Capacitor_t *cap, const Referee_ForCap_t *referee,
                 CAN_CapOutput_t *cap_out);
#ifdef __cplusplus
}
#endif
