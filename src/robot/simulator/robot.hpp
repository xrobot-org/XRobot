#include "comp_utils.hpp"
#include "om.hpp"
#include "term.hpp"

void robot_init();

namespace Robot {
class Simulator {
 public:
  Message message_;

  System::Term term_;

  Simulator() {}
};
}  // namespace Robot
