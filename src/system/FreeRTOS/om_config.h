/* Debug */
#if USE_FULL_ASSERT
#define OM_DEBUG (1)
#else
#define OM_DEBUG (0)
#endif

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
#define om_mutex_init(arg)         \
  *arg = xSemaphoreCreateBinary(); \
  xSemaphoreGive(*arg)
#define om_mutex_lock(arg) xSemaphoreTake(*arg, portMAX_DELAY)
#define om_mutex_trylock(arg) \
  (xSemaphoreTake(*arg, 0) == pdTRUE ? OM_OK : OM_ERROR)
#define om_mutex_unlock(arg) xSemaphoreGive(*arg)

#define om_mutex_lock_isr(arg) \
  (xSemaphoreTakeFromISR(*arg, NULL) == pdTRUE ? OM_OK : OM_ERROR)
#define om_mutex_unlock_isr(arg) xSemaphoreGiveFromISR(*arg, NULL)
#define om_mutex_delete(arg) vSemaphoreDelete(*arg)

/* 将运行时间作为消息发出的时间 */
#define OM_TIME (0)

/* 开启"om_log"话题作为OneMessage的日志输出 */
#define OM_LOG_OUTPUT (1)

#if OM_LOG_OUTPUT
/* 按照日志等级以不同颜色输出 */
#define OM_LOG_COLORFUL (1)
/* 日志最大长度 */
#define OM_LOG_MAX_LEN (80)
/* 日志等级 1:default 2:notice 3:pass 4:warning 5:error  */
#define OM_LOG_LEVEL (1)
#endif

/* 话题名称最大长度 */
#define OM_TOPIC_MAX_NAME_LEN (25)

#include "bsp_sys.h"
static inline bool om_in_isr() { return bsp_sys_in_isr(); }
