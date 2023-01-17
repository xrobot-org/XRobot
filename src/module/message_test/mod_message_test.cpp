#include "mod_message_test.hpp"

#include <thread.hpp>

using namespace Module;

MessageTest::MessageTest(Param& param) : param_(param), topic_("test_topic") {
  auto pub_thread_fn = [](MessageTest* msg_test) {
    while (1) {
      msg_test->pub_data_.d2++;
      msg_test->topic_.Publish(msg_test->pub_data_);
      msg_test->pub_thread_.SleepUntil(1);
    }
  };
  this->pub_thread_.Create(pub_thread_fn, this, "msg_test_pub_thread", 256,
                           System::Thread::MEDIUM);

  auto sub_thread_fn = [](MessageTest* msg_test) {
    auto tp_sub = Message::Subscriber<Data>("test_topic", msg_test->pub_data_);
    while (1) {
      tp_sub.DumpData();
      printf("%d\n", msg_test->pub_data_.d2);
      msg_test->pub_thread_.SleepUntil(1);
    }
  };
  this->pub_thread_.Create(sub_thread_fn, this, "msg_test_sub_thread", 256,
                           System::Thread::MEDIUM);
}
