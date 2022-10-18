#include "dev_term.hpp"

#include "bsp_usb.h"

using namespace Device;

Term::Term() {
  bsp_usb_init();

  auto term_thread = [](Term *term) {
    while (1) {
      term->Update();
    }
  };

  this->thread_.Create(term_thread, this, "term_thread", 256,
                       System::Thread::Realtime);
}

bool Term::Update() {
  bsp_usb_update();
  return true;
}

bool Term::Opened() { return bsp_usb_connect(); }

uint32_t Term::Available() { return bsp_usb_avail(); }

char Term::ReadChar() { return bsp_usb_read_char(); }

uint32_t Term::Read(uint8_t *buffer, uint32_t len) {
  return bsp_usb_read(buffer, len);
}

bool Term::Write(uint8_t *buffer, uint32_t len) {
  return bsp_usb_transmit(buffer, len) == BSP_OK;
}
