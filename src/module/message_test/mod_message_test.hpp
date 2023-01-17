#include <module.hpp>
#include <thread.hpp>

namespace Module {
class MessageTest {
 public:
  struct Param {};

  struct Data {
    float d1;
    int d2;
    char d3;
  };

  MessageTest(Param& param);

  Param& param_;

  Data pub_data_;

  Data sub_data_;

  Message::Topic<Data> topic_;

  System::Thread pub_thread_, sub_thread_;
};
}  // namespace Module
