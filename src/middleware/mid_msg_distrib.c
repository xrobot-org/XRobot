/**
 * @file mid_msg_distrib.h
 * @author Qu Shen
 * @brief 消息分发中间件
 * @version 0.1
 * @date 2021-09-04
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "mid_msg_distrib.h"

#include <stddef.h>
#include <string.h>

#include "FreeRTOS.h"
#include "comp_utils.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#define MAX_NAME_LEN (20)
#define MAX_TOPIC (10)
#define MAX_SUBS_TO_ONE_TPIC (5)

/* 话题监控 */
typedef struct {
  uint32_t pub_freq;
  uint32_t total_pub;
  uint32_t online_tick;
  uint32_t unexpected;
} MsgDistrib_TopicMonitor_t;

/* 话题 */
typedef struct {
  char name[MAX_NAME_LEN];
  void *data_buf;
  size_t data_size;
  MsgDistrib_Publisher_t pub;
  MsgDistrib_Subscriber_t subs[MAX_SUBS_TO_ONE_TPIC];
  uint32_t num_pubs;
  uint32_t num_subs;

  MsgDistrib_TopicMonitor_t monitor;
} MsgDistrib_Topic_t;

static struct {
  QueueSetHandle_t topic_queue_set;
  MsgDistrib_Topic_t topic_list[MAX_TOPIC];
  SemaphoreHandle_t topic_sem;
} md;

/**
 * @brief 初始化中间件
 *
 * @return true 成功
 * @return false 失败
 */
bool MsgDistrib_Init(void) {
  if (md.topic_queue_set == NULL) {
    md.topic_queue_set = xQueueCreateSet(MAX_SUBS_TO_ONE_TPIC);
    md.topic_sem = xSemaphoreCreateCounting(MAX_SUBS_TO_ONE_TPIC, 0);
    return true;
  }
  return false;
}

/**
 * @brief 创建话题
 *
 * @param topic_name 话题名称
 * @param data_size 数据大小
 * @return MsgDistrib_Publisher_t* 发布者
 */
MsgDistrib_Publisher_t *MsgDistrib_CreateTopic(const char *topic_name,
                                               size_t data_size) {
  ASSERT(topic_name);
  while (md.topic_sem == NULL) {
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  if (xSemaphoreTake(md.topic_sem, portMAX_DELAY)) {
    for (size_t i = 0; i < MAX_TOPIC; i++) {
      MsgDistrib_Topic_t *topic = md.topic_list + i;
      if (topic->name == NULL) {
        strncpy(topic_name, topic->name, MAX_NAME_LEN - 1);
        topic->name[MAX_NAME_LEN] = '\0';
        topic->data_size = data_size;
        topic->data_buf = pvPortMalloc(data_size);

        topic->pub.data_queue = xQueueCreate(1, data_size);
        xQueueAddToSet(topic->pub.data_queue, md.topic_queue_set);

        topic->monitor.online_tick = xTaskGetTickCount();

        return &(topic->pub);
      }
    }
    xSemaphoreGive(md.topic_sem);
  }
  return NULL;
}

/**
 * @brief 往话题发布数据
 *
 * @param publisher 发布者
 * @param data 要发布的数据
 * @return true 成功
 * @return false 失败
 */
bool MsgDistrib_Publish(MsgDistrib_Publisher_t *publisher, void *data) {
  ASSERT(publisher);
  ASSERT(data);

  MsgDistrib_Topic_t *topic = CONTAINER_OF(publisher, MsgDistrib_Topic_t, pub);

  topic->monitor.total_pub++;
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
bool MsgDistrib_PublishFromISR(MsgDistrib_Publisher_t *publisher,
                               const void *data, bool *need_contex_switch) {
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
 * @return MsgDistrib_Subscriber_t*
 */
MsgDistrib_Subscriber_t *MsgDistrib_Subscribe(const char *topic_name,
                                              bool wait_topic) {
  ASSERT(topic_name);
  while (wait_topic) {
    for (size_t i = 0; i < MAX_TOPIC; i++) {
      MsgDistrib_Topic_t *topic = md.topic_list + i;
      if (strncmp(topic->name, topic_name, MAX_NAME_LEN) == 0) {
        for (size_t j = 0; j < MAX_SUBS_TO_ONE_TPIC; j++) {
          if (topic->subs[j].data_queue == NULL) {
            topic->subs[j].data_queue = xQueueCreate(1, topic->data_size);
            topic->subs[j].bin_sem = xSemaphoreCreateBinary();
          }
          return &(topic->pub);
        }
        return NULL;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

/**
 * @brief 从话题去的消息
 *
 * @param subscriber 订阅者
 * @param data 数据
 * @param timeout 等待时间
 * @return true 成功
 * @return false 失败
 */
bool MsgDistrib_Poll(MsgDistrib_Subscriber_t *subscriber, void *data,
                     uint32_t timeout) {
  ASSERT(subscriber);
  ASSERT(data);
  BaseType_t ret;
  if (timeout) {
    ret = xSemaphoreTake(subscriber->bin_sem, pdMS_TO_TICKS(timeout));
  }
  if ((ret == pdTRUE) || (!timeout)) {
    ret = xQueuePeek(subscriber->data_queue, data, 0);
    if (ret == pdTRUE) return true;
  }

  MsgDistrib_Topic_t *topic =
      CONTAINER_OF(subscriber, MsgDistrib_Topic_t, subs);
  topic->monitor.unexpected++;
  return false;
}

/**
 * @brief 分法消息
 *
 */
void MsgDistrib_Distribute(void) {
  /* 阻塞在所有话题的发布者 */
  QueueSetMemberHandle_t actived =
      xQueueSelectFromSet(md.topic_queue_set, portMAX_DELAY);

  for (size_t i = 0; i < MAX_TOPIC; i++) {
    MsgDistrib_Topic_t *topic = md.topic_list + i;

    /* 确认话题发布者 */
    if (topic->pub.data_queue == actived) {
      MsgDistrib_TopicMonitor_t *monitor = &(topic->monitor);
      BaseType_t ret;

      /* 接收发布者的数据 */
      ret = xQueueReceive(topic->pub.data_queue, topic->data_buf, 0);
      if (ret != pdPASS) monitor->unexpected++;

      /* 分发给订阅者 */
      for (size_t j = 0; j < MAX_SUBS_TO_ONE_TPIC; j++) {
        if (topic->subs[j].data_queue != NULL) {
          vTaskSuspendAll();
          xQueueOverwrite(topic->subs[j].data_queue, topic->data_buf);
          xSemaphoreGive(topic->subs[j].bin_sem);
          vTaskResumeAll();
        }
      }

      /* 监控话题运行 */
      if (monitor->total_pub) {
        monitor->pub_freq =
            (xTaskGetTickCount() - monitor->online_tick) / monitor->total_pub;
      } else {
        monitor->pub_freq = 0;
      }
    }
  }
}

void MsgDistrib_Detail(char *detail_string, size_t len) {
  static const char *const header = "\r\n";
  strncpy(header, detail_string, len - 1);
  detail_string[len] = '\0';
}
