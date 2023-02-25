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

  List() { INIT_LIST_HEAD(&(this->head_)); }

  bool Add(Data data) {
    Node* node = static_cast<Node*>(pvPortMalloc(sizeof(Node)));
    memcpy(&(node->data_), &data, sizeof(data));
    om_list_add(&(node->node_), &(this->head_));

    return true;
  }

  bool AddTail(Data data) {
    Node* node = static_cast<Node*>(pvPortMalloc(sizeof(Node)));
    memcpy(&(node->data_), &data, sizeof(data));
    om_list_add_tail(&(node->node_), (this->head_));

    return true;
  }

  void Foreach(bool (*fun)(Data&, void*), void* arg) {
    om_list_head_t* pos = NULL;
    om_list_for_each(pos, &(this->head_)) {
      Node* data = om_list_entry(pos, Node, node_);
      if (!fun(data->data_, arg)) {
        break;
      }
    }
  }

  om_list_head_t head_;
};
}  // namespace System
