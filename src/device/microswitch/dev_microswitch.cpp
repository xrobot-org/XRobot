#include "dev_microswitch.hpp"

#include "dev_can.hpp"

using namespace Device;

bool MicroSwitch::inited_ = false;
Message::Topic<MicroSwitch::Data> *MicroSwitch::topic_;

MicroSwitch::MicroSwitch(Param &param) : param_(param) {
  if (!inited_) {
    topic_ = new Message::Topic<Data>("micro_switch_data");
  }

  inited_ = true;

  auto rx_callback = [](Can::Pack &rx, MicroSwitch *sw) {
    if (rx.index == sw->param_.id) {
      for (int i = 0; i < 4; i++) {
        if (rx.data[i * 2] == i) {
          sw->data_.id = sw->param_.id * 4 + i;
          sw->data_.status = static_cast<Status>(rx.data[i * 2 + 1]);
          sw->topic_->Publish(sw->data_);
        }
      }
    }
    return true;
  };

  Message::Topic<Can::Pack> sw_tp(
      (std::string("can_sw_") + std::to_string(param.id)).c_str());
  sw_tp.RegisterCallback(rx_callback, this);

  Can::Subscribe(sw_tp, this->param_.can, this->param_.id, 1);
}

void MicroSwitch::Subscriber(Data &data, uint8_t switch_id) {
  data.id = switch_id;
  auto cb_fn = [](Data &raw_data, Data *data) {
    if (data->id == raw_data.id) {
      data->status = raw_data.status;
    }
    return true;
  };

  topic_->RegisterCallback(cb_fn, &data);
}
