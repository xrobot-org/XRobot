#pragma once

#include <mutex.hpp>

#include "om.h"
#include "om_list.h"

namespace System {
template <typename Data>
class List {
 public:
  typedef struct {
    Data data_;
    om_list_head_t node_;
  } Node;

  List() { OM_INIT_LIST_HEAD(&(this->head_)); }

  bool Add(Node& node) {
    mutex_.Lock();
    om_list_add(&(node.node_), &(this->head_));
    mutex_.Unlock();

    return true;
  }

  void Delete(Node& node) {
    mutex_.Lock();
    om_list_del(node.node_.next);
    mutex_.Unlock();
  }

  void Foreach(bool (*fun)(Data&, void*), void* arg) {
    mutex_.Lock();
    om_list_head_t* pos = NULL;
    om_list_for_each(pos, &(this->head_)) {
      Node* data = om_list_entry(pos, Node, node_);
      if (!fun(data->data_, arg)) {
        break;
      }
    }
    mutex_.Unlock();
  }

  om_list_head_t head_;
  System::Mutex mutex_;
};
}  // namespace System
