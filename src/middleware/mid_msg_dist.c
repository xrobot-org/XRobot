#include "mid_msg_dist.h"

#include <stddef.h>
#include <string.h>

#include "FreeRTOS.h"
#include "comp_utils.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#define MAX_NAME_LEN (20)
#define MAX_TOPIC (40)           /* topic上限数量 */
#define MAX_SUBS_TO_ONE_TPIC (5) /* 每个topic的subscriber上限 */

/* 话题监控 */
typedef struct {
  uint32_t pub_freq;
  uint32_t num_pub;
  uint32_t num_suber;
  uint32_t created_tick;
  uint32_t unexpected;
} MsgDist_TopicMonitor_t;

/* 话题 */
typedef struct {
  char name[MAX_NAME_LEN];
  void *data_buf;
  size_t data_size;
  publisher_t puber;
  subscriber_t subers[MAX_SUBS_TO_ONE_TPIC];

  MsgDist_TopicMonitor_t monitor;
} MsgDist_Topic_t;

/* 消息分发 */
static struct {
  QueueSetHandle_t topic_queue_set;
  MsgDist_Topic_t topic_list[MAX_TOPIC];
  SemaphoreHandle_t topic_mutex;
  size_t topic_created;
} md;

/**
 * @brief 初始化中间件
 *
 * @return true 成功
 * @return false 失败
 */
bool msg_dist_init(void) {
  /* 只能初始化一次 */
  if (md.topic_queue_set == NULL) {
    md.topic_queue_set = xQueueCreateSet(MAX_TOPIC);
    md.topic_mutex = xSemaphoreCreateMutex();
    md.topic_created = 0;
    return true;
  }
  return false;
}

/**
 * @brief 创建话题
 *
 * @param topic_name 话题名称
 * @param data_size 数据大小
 * @return publisher_t* 发布者
 */
publisher_t *msg_dist_create_topic(const char *topic_name, size_t data_size) {
  ASSERT(topic_name);

  /* 等待组件初始化 */
  while (md.topic_mutex == NULL) {
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  publisher_t *puber = NULL;
  MsgDist_Topic_t *topic;

  xSemaphoreTake(md.topic_mutex, portMAX_DELAY);
  if (md.topic_created < MAX_TOPIC) {
    for (size_t i = 0; i < md.topic_created; i++) {
      topic = md.topic_list + i;
      if (strncmp(topic->name, topic_name, MAX_NAME_LEN) == 0) {
        /* 检查是否有重名topic */
        puber = NULL;
        goto end;
      }
    }
    topic = md.topic_list + md.topic_created;
    memset(topic->name, 0, sizeof(topic->name));
    strncpy(topic->name, topic_name, MAX_NAME_LEN - 1);
    topic->data_size = data_size;
    topic->data_buf = pvPortMalloc(data_size);
    memset(topic->data_buf, 0, data_size);

    topic->puber.data_queue = xQueueCreate(1, data_size);
    xQueueAddToSet(topic->puber.data_queue, md.topic_queue_set);

    topic->monitor.created_tick = xTaskGetTickCount();

    md.topic_created++;
    puber = &(topic->puber);
  } else {
    puber = NULL;
  }

end:
  xSemaphoreGive(md.topic_mutex);
  return puber;
}

/**
 * @brief 往话题发布数据
 *
 * @param publisher 发布者
 * @param data 要发布的数据
 * @return true 成功
 * @return false 失败
 */
bool msg_dist_publish(publisher_t *publisher, void *data) {
  ASSERT(publisher);
  ASSERT(data);

  MsgDist_Topic_t *topic = CONTAINER_OF(publisher, MsgDist_Topic_t, puber);

  topic->monitor.num_pub++;
  return (xQueueOverwrite(publisher->data_queue, data) == pdPASS);
}

/**
 * @brief 往话题发布数据
 *
 * @param publisher 发布者
 * @param data 要发布的数据
 * @param need_contex_switch 会不会导致高优先级线程唤醒
 * @return true 成功
 * @return false 失败
 */
bool msg_dist_publish_from_isr(publisher_t *publisher, const void *data,
                               bool *need_contex_switch) {
  ASSERT(publisher);
  ASSERT(data);
  BaseType_t need_switch;
  BaseType_t send_result =
      xQueueSendFromISR(publisher->data_queue, data, &need_switch);
  *need_contex_switch = (need_switch == pdTRUE);
  return (send_result == pdPASS);
}

/**
 * @brief 订阅到话题
 *
 * @param topic_name 话题名称
 * @param wait_topic 等待话题创建
 * @return subscriber_t*
 */
subscriber_t *msg_dist_subscribe(const char *topic_name, bool wait_topic) {
  ASSERT(topic_name);
  do {
    for (size_t i = 0; i < MAX_TOPIC; i++) {
      MsgDist_Topic_t *topic = md.topic_list + i;
      if (strncmp(topic->name, topic_name, MAX_NAME_LEN) == 0) {
        for (size_t j = 0; j < MAX_SUBS_TO_ONE_TPIC; j++) {
          if (topic->subers[j].bin_sem == NULL) {
            topic->subers[j].bin_sem = xSemaphoreCreateBinary();
            topic->subers[j].topic = topic;
            topic->monitor.num_suber++;
            return topic->subers + j;
          }
        }
        return NULL;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  } while (wait_topic);
  return NULL;
}

/**
 * @brief 从话题取得数据
 *
 * @param subscriber 订阅者
 * @param data 数据
 * @param timeout 等待时间
 * @return true 成功
 * @return false 失败
 */
bool msg_dist_poll(subscriber_t *subscriber, void *data, uint32_t timeout) {
  ASSERT(subscriber);
  ASSERT(data);
  MsgDist_Topic_t *topic = subscriber->topic;

  BaseType_t ret = pdFALSE;
  if (timeout) {
    ret = xSemaphoreTake(subscriber->bin_sem, pdMS_TO_TICKS(timeout));
  } else {
    ret = pdTRUE;
  }

  if (ret == pdTRUE) {
    memcpy(data, topic->data_buf, topic->data_size);
    return true;
  }
  topic->monitor.unexpected++;
  return false;
}

/**
 * @brief 分发消息
 *
 */
void msg_dist_distribute(void) {
  /* 阻塞在所有话题的发布者 */
  QueueSetMemberHandle_t actived =
      xQueueSelectFromSet(md.topic_queue_set, portMAX_DELAY);

  for (size_t i = 0; i < MAX_TOPIC; i++) {
    MsgDist_Topic_t *topic = md.topic_list + i;

    /* 确认话题发布者 */
    if (topic->puber.data_queue == actived) {
      MsgDist_TopicMonitor_t *monitor = &(topic->monitor);
      BaseType_t ret;

      /* 接收发布者的数据 */
      ret = xQueueReceive(topic->puber.data_queue, topic->data_buf, 0);
      if (ret != pdPASS) monitor->unexpected++;

      /* 分发给订阅者 */
      for (size_t j = 0; j < MAX_SUBS_TO_ONE_TPIC; j++) {
        if (topic->subers[j].bin_sem != NULL) {
          xSemaphoreGive(topic->subers[j].bin_sem);
        }
      }

      /* 监控话题运行 */
      if (monitor->num_pub) {
        monitor->pub_freq =
            (xTaskGetTickCount() - monitor->created_tick) / monitor->num_pub;
      } else {
        monitor->pub_freq = 0;
      }
    }
  }
}

// TODO: Move to CLI
#include <stdio.h>
void msg_dist_detail(char *detail_string, size_t len) {
  snprintf(detail_string, len - 1, "num topics: %d", md.topic_created);

  static const char *const topic_header =
      "name                 #pub  pub_freq  #suber  data_size  created at\r\n";

  strncpy(detail_string, topic_header, len - 1);

  snprintf(detail_string, len - 1, " %20s %4ld  %8ld  %6ld  %9d  %10ld",
           md.topic_list[0].name, md.topic_list[0].monitor.num_pub,
           md.topic_list[0].monitor.pub_freq,
           md.topic_list[0].monitor.num_suber, md.topic_list[0].data_size,
           md.topic_list[0].monitor.created_tick);

  detail_string[len] = '\0';
}
