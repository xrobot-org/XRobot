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
} MsgDistrib_Publisher_t;

/* 订阅者 */
typedef struct {
  SemaphoreHandle_t bin_sem;
  QueueHandle_t data_queue;
} MsgDistrib_Subscriber_t;

/**
 * @brief 初始化中间件
 *
 * @return true 成功
 * @return false 失败
 */
bool MsgDistrib_Init(void);

/**
 * @brief 创建话题
 *
 * @param topic_name 话题名称
 * @param data_size 数据大小
 * @return MsgDistrib_Publisher_t* 发布者
 */
MsgDistrib_Publisher_t *MsgDistrib_CreateTopic(const char *topic_name,
                                               size_t data_size);
/**
 * @brief 往话题发布数据
 *
 * @param publisher 发布者
 * @param data 要发布的数据
 * @return true 成功
 * @return false 失败
 */
bool MsgDistrib_Publish(MsgDistrib_Publisher_t *publisher, void *data);
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
                               const void *data, bool *need_contex_switch);

/**
 * @brief 订阅到话题
 *
 * @param topic_name 话题名称
 * @param wait_topic 等待话题创建
 * @return MsgDistrib_Subscriber_t*
 */
MsgDistrib_Subscriber_t *MsgDistrib_Subscribe(const char *topic_name,
                                              bool wait_topic);
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
                     uint32_t timeout);
/**
 * @brief 分法消息
 *
 */
void MsgDistrib_Distribute(void);
void MsgDistrib_Detail(char *detail_string, size_t len);