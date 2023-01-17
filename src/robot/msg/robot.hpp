#include "mod_message_test.hpp"

void robot_init();

namespace Robot {
class MSG {
 public:
  struct Param {
    Module::MessageTest::Param msg_test;
  };

  Module::MessageTest msg_test_;

  MSG(Param& param) : msg_test_(param.msg_test) {}
};
}  // namespace Robot
