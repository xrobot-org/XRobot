/* Debug */
#if USE_FULL_ASSERT
#define OM_DEBUG (1)
#else
#define OM_DEBUG (0)
#endif

/* 使用用户自定义的内存分配 */
#define OM_USE_USER_MALLOC (0)

/* 用户内存分配函数 */
#if OM_USE_USER_MALLOC
#include "FreeRTOS.h"
#define om_malloc pvPortMalloc
#define om_free vPortFree
#endif

/* 非阻塞延时函数 */
#include "bsp_delay.h"
#define om_delay_ms bsp_delay

/* OS层互斥锁api */
#define om_mutex_t uint8_t
#define om_mutex_init(arg) (void)arg, OM_OK
#define om_mutex_lock(arg) (void)arg, OM_OK
#define om_mutex_trylock(arg) (void)arg, OM_OK
#define om_mutex_unlock(arg) (void)arg, OM_OK

#define om_mutex_lock_isr(arg) (void)arg, OM_OK
#define om_mutex_unlock_isr(arg) (void)arg, OM_OK
#define om_mutex_delete(arg) (void)arg, OM_OK

/* 将运行时间作为消息发出的时间 */
#define OM_TIME (0)

/* 开启"om_log"话题作为OneMessage的日志输出 */
#define OM_LOG_OUTPUT (1)

#if OM_LOG_OUTPUT
/* 按照日志等级以不同颜色输出 */
#define OM_LOG_COLORFUL (1)
/* 日志最大长度 */
#define OM_LOG_MAX_LEN (60)
/* 日志等级 1:default 2:notice 3:pass 4:warning 5:error  */
#define OM_LOG_LEVEL (1)
#endif

/* 话题名称最大长度 */
#define OM_TOPIC_MAX_NAME_LEN (25)
