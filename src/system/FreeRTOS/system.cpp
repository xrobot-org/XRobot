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
  Message* msg = static_cast<Message*>(pvPortMalloc(sizeof(Message)));
  new (msg) Message();
  Term* term = static_cast<Term*>(pvPortMalloc(sizeof(Term)));
  new (term) Term();
  Database* database = static_cast<Database*>(pvPortMalloc(sizeof(Database)));
  new (database) Database();
  Timer* timer = static_cast<Timer*>(pvPortMalloc(sizeof(Timer)));
  new (timer) Timer();
}
