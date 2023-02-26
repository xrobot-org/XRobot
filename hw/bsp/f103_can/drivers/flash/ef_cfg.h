#ifndef EF_CFG_H_
#define EF_CFG_H_

/* using ENV function, default is NG (Next Generation) mode start from V4.0 */
#define EF_USING_ENV

#ifdef EF_USING_ENV
/* Auto update ENV to latest default when current ENV version number is changed.
 */
/* #define EF_ENV_AUTO_UPDATE */
/**
 * ENV version number defined by user.
 * Please change it when your firmware add a new ENV to default_env_set.
 */
#define EF_ENV_VER_NUM 0 /* @note you must define it for a value, such as 0 */

/* MCU Endian Configuration, default is Little Endian Order. */
/* #define EF_BIG_ENDIAN  */

#endif /* EF_USING_ENV */

/* using IAP function */
/* #define EF_USING_IAP */

/* using save log function */
/* #define EF_USING_LOG */

/* The minimum size of flash erasure. May be a flash sector size. */
#define EF_ERASE_MIN_SIZE 1024 /* @note you must define it for a value */

/* the flash write granularity, unit: bit
 * only support 1(nor flash)/ 8(stm32f4)/ 32(stm32f1) */
#define EF_WRITE_GRAN 32 /* @note you must define it for a value */

/*
 *
 * This all Backup Area Flash storage index. All used flash area configure is
 * under here.
 * |----------------------------|   Storage Size
 * | Environment variables area |   ENV area size @see ENV_AREA_SIZE
 * |----------------------------|
 * |      Saved log area        |   Log area size @see LOG_AREA_SIZE
 * |----------------------------|
 * |(IAP)Downloaded application |   IAP already downloaded application, unfixed
 * size
 * |----------------------------|
 *
 * @note all area sizes must be aligned with EF_ERASE_MIN_SIZE
 *
 * The EasyFlash add the NG (Next Generation) mode start from V4.0. All old mode
 * before V4.0, called LEGACY mode.
 *
 * - NG (Next Generation) mode is default mode from V4.0. It's easy to settings,
 * only defined the ENV_AREA_SIZE.
 * - The LEGACY mode has been DEPRECATED. It is NOT RECOMMENDED to continue
 * using. Beacuse it will use ram to buffer the ENV and spend more flash erase
 * times. If you want use it please using the V3.X version.
 */

/* backup area start address */
#define EF_START_ADDR                 \
  (0x08000000UL + EF_ERASE_MIN_SIZE * \
                      62) /* 62k */ /* @note you must define it for a value */

/* ENV area size. It's at least one empty sector for GC. So it's definition must
 * more then or equal 2 flash sector size. */
#define ENV_AREA_SIZE                                                         \
  (2 * EF_ERASE_MIN_SIZE) /* 2k */ /* @note you must define it for a value if \
                                      you used ENV */

/* saved log area size */

#endif /* EF_CFG_H_ */
