#include "system.hpp"

#include "database.hpp"
#include "memory.hpp"
#include "om.hpp"
#include "queue.hpp"
#include "semaphore.hpp"
#include "term.hpp"
#include "thread.hpp"
#include "timer.hpp"

using namespace System;

void System::Init() {
  new Message();
  new Term();
  new Database();
  new Timer();
}
