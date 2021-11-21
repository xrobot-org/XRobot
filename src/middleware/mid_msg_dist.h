#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

/* 发布者 */
typedef struct {
  QueueHandle_t data_queue;
} publisher_t;

/* 订阅者 */
typedef struct {
  SemaphoreHandle_t bin_sem;
  void *topic;
} subscriber_t;

/**
 * @brief 初始化中间件
 *
 * @return true 成功
 * @return false 失败
 */
bool msg_dist_init(void);

/**
 * @brief 创建话题
 *
 * @param topic_name 话题名称
 * @param data_size 数据大小
 * @return publisher_t* 发布者
 */
publisher_t *msg_dist_create_topic(const char *topic_name, size_t data_size);
/**
 * @brief 往话题发布数据
 *
 * @param publisher 发布者
 * @param data 要发布的数据
 * @return true 成功
 * @return false 失败
 */
bool msg_dist_publish(publisher_t *publisher, void *data);
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
                               bool *need_contex_switch);

/**
 * @brief 订阅到话题
 *
 * @param topic_name 话题名称
 * @param wait_topic 等待话题创建
 * @return subscriber_t* 订阅者
 */
subscriber_t *msg_dist_subscribe(const char *topic_name, bool wait_topic);
/**
 * @brief 从话题取得数据
 *
 * @param subscriber 订阅者
 * @param data 数据
 * @param timeout 等待时间
 * @return true 成功
 * @return false 失败
 */
bool msg_dist_poll(subscriber_t *subscriber, void *data, uint32_t timeout);
/**
 * @brief 分发消息
 *
 */
void msg_dist_distribute(void);

void msg_dist_detail(char *detail_string, size_t len);
