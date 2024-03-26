#include "comp_ui.hpp"
#include "device.hpp"

namespace Device {
class CustomController {
 public:
  CustomController();

  typedef enum { NUM = 144 } ControllerEvent;

  bool StartRecv();

  void Prase();

  void Offline();

 private:
  System::Semaphore packet_recv_ = System::Semaphore(false);

  Message::Topic<Component::CMD::Data> controller_angel_ =
      Message::Topic<Component::CMD::Data>("collectangle");

  Message::Event event_;
  System::Thread recv_thread_;
  System::Thread trans_thread_;
  Component::CMD::Data controller_data_{};
};
}  // namespace Device
