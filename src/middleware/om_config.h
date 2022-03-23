/* Debug */
#if USE_FULL_ASSERT
#define OM_DEBUG (1)
#else
#define OM_DEBUG (0)
#endif

/* OneMessage主线程调用的频率 */
#define OM_CALL_FREQ (1000)

/* 使能这个宏可能会提高刷新频率的精度，但会消耗更多性能 */
#define OM_FREQ_USE_FLOAT (0)

/* 严格限制导出数据时的长度 */
#define OM_STRICT_LIMIT (1)

/* 使用用户自定义的内存分配 */
#define OM_USE_USER_MALLOC (1)

/* 用户内存分配函数 */
#if OM_USE_USER_MALLOC
#include "FreeRTOS.h"
#define om_malloc pvPortMalloc
#define om_free vPortFree
#endif

/* 非阻塞延时函数 */
#define om_delay_ms vTaskDelay

/* OS层互斥锁api */
#include "semphr.h"
#define om_mutex_t SemaphoreHandle_t
#define om_mutex_init(arg)        \
  *arg = xSemaphoreCreateMutex(); \
  xSemaphoreGive(*arg)
#define om_mutex_lock(arg) xSemaphoreTake(*arg, portMAX_DELAY)
#define om_mutex_trylock(arg) \
  xSemaphoreTake(*arg, 0) == pdTRUE ? OM_OK : OM_ERROR
#define om_mutex_unlock(arg) xSemaphoreGive(*arg)

/* 将运行时间作为消息发出的时间 */
#define OM_VIRTUAL_TIME (1)

#if !OM_VIRTUAL_TIME
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
#define OM_LOG_MAX_LEN (60)
#endif

/* 话题名称最大长度 */
#define OM_TOPIC_MAX_NAME_LEN (20)
