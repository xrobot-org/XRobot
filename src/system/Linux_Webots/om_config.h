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
#define om_malloc user_malloc
#define om_free user_free
#endif

/* 非阻塞延时函数 */
#include <poll.h>
#include <pthread.h>
#include <stdio.h>

#define om_delay_ms(_arg) poll(NULL, 0, _arg)

/* OS层互斥锁api */
#include <pthread.h>
#define om_mutex_t pthread_mutex_t
#define om_mutex_init(arg) pthread_mutex_init(arg, NULL)
#define om_mutex_lock(arg) pthread_mutex_lock(arg)
#define om_mutex_trylock(arg) pthread_mutex_trylock(arg) == 0 ? OM_OK : OM_ERROR
#define om_mutex_unlock(arg) pthread_mutex_unlock(arg)

#define om_mutex_lock_isr(arg) pthread_mutex_lock(arg)
#define om_mutex_unlock_isr(arg) pthread_mutex_unlock(arg)

#define om_mutex_delete(arg) pthread_mutex_destroy(arg)

/* 将运行时间作为消息发出的时间 */
#define OM_TIME (1)

#if OM_TIME
#include <time.h>
#define om_time_t time_t
#define om_time_get(_time) time(_time)
#endif

/* 开启"om_log"话题作为OneMessage的日志输出 */
#define OM_LOG_OUTPUT (1)

#if OM_LOG_OUTPUT
/* 按照日志等级以不同颜色输出 */
#define OM_LOG_COLORFUL (1)
/* 日志最大长度 */
#define OM_LOG_MAX_LEN (120)
/* 日志等级 1:default 2:notice 3:pass 4:warning 5:error  */
#define OM_LOG_LEVEL (1)
#endif

/* 话题名称最大长度 */
#define OM_TOPIC_MAX_NAME_LEN (25)

#include "bsp_sys.h"
static inline bool om_in_isr() { return bsp_sys_in_isr(); }
