#pragma once

#ifdef __cplusplus
 extern "C" {
#endif

#define DEVICE_OK			(0)
#define DEVICE_ERR			(-1)
#define DEVICE_ERR_NULL		(-2)
#define DEVICE_ERR_INITED	(-3)
#define DEVICE_ERR_NO_DEV	(-4)

#define SIGNAL_BMI088_GYRO_NEW_DATA		(1u<<0)
#define SIGNAL_BMI088_ACCL_NEW_DATA		(1u<<1)
#define SIGNAL_BMI088_GYRO_RAW_REDY		(1u<<2)
#define SIGNAL_BMI088_ACCL_RAW_REDY		(1u<<3)

#define SIGNAL_CAN_MOTOR_RECV			(1u<<4)
#define SIGNAL_CAN_UWB_RECV				(1u<<5)
#define SIGNAL_CAN_CAP_RECV				(1u<<6)

#define SIGNAL_DR16_RAW_REDY			(1u<<7)

#define SIGNAL_IST8310_MAGN_RAW_REDY	(1u<<8)

#define SIGNAL_REFEREE_RAW_REDY			(1u<<9)

#ifdef __cplusplus
}
#endif
