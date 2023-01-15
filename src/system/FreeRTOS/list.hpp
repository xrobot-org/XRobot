#pragma once

#include "FreeRTOS.h"
#include "om.h"

namespace System {
template <typename Data>
class List {
 public:
  typedef struct {
    Data data_;
    om_list_head_t node_;
  } Node;

  List() {
    INIT_LIST_HEAD(&(this->head_));
    this->sem_handle_ = xSemaphoreCreateBinary();
    xSemaphoreGive(this->sem_handle_);
  }

  bool Add(Data data) {
    xSemaphoreTake(this->sem_handle_, UINT32_MAX);
    Node* node = static_cast<Node*>(pvPortMalloc(sizeof(Node)));
    memcpy(&(node->data_), &data, sizeof(data));
    om_list_add(&(node->node_), &(this->head_));
    xSemaphoreGive(this->sem_handle_);

    return true;
  }

  bool AddTail(Data data) {
    xSemaphoreTake(this->sem_handle_, UINT32_MAX);
    Node* node = static_cast<Node*>(pvPortMalloc(sizeof(Node)));
    memcpy(&(node->data_), &data, sizeof(data));
    om_list_add_tail(&(node->node_), (this->head_));
    xSemaphoreGive(this->sem_handle_);

    return true;
  }

  void Foreach(bool (*fun)(Data&, void*), void* arg) {
    xSemaphoreTake(this->sem_handle_, UINT32_MAX);
    om_list_head_t* pos;
    om_list_for_each(pos, &(this->head_)) {
      Node* data = om_list_entry(pos, Node, node_);
      if (!fun(data->data_, arg)) break;
    }
    xSemaphoreGive(this->sem_handle_);
  }

  om_list_head_t head_;
  SemaphoreHandle_t sem_handle_;
};
}  // namespace System
