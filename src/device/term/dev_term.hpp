#include "dev.hpp"

namespace Device {
class Term {
 public:
  Term();

  bool Update();

  bool Opened();

  uint32_t Available();

  char ReadChar();

  uint32_t Read(uint8_t *buffer, uint32_t len);

  bool Write(uint8_t *buffer, uint32_t len);

  System::Thread thread_;
};
}  // namespace Device
