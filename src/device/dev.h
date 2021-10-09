#pragma once

#define DEVICE_OK (0)
#define DEVICE_ERR (-1)
#define DEVICE_ERR_NULL (-2)
#define DEVICE_ERR_INITED (-3)
#define DEVICE_ERR_NO_DEV (-4)

/*
  SIGNAL是用bit位置区分的，一个SIGNAL只能包含一个高bit位。
  不同的SIGNAL不能有相同的高bit位。
*/
#define SIGNAL_CAN_MOTOR_RECV (1u << 4)
#define SIGNAL_CAN_UWB_RECV (1u << 5)
#define SIGNAL_CAN_CAP_RECV (1u << 6)

#define SIGNAL_DR16_RAW_REDY (1u << 7)
#define SIGNAL_IST8310_MAGN_NEW_DATA (1u << 8)
#define SIGNAL_IST8310_MAGN_RAW_REDY (1u << 9)
