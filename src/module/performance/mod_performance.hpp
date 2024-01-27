#include "bsp_time.h"
#include "module.hpp"

namespace Module {
class Performance {
 public:
  System::Thread thread_test;

  System::Term::Command<Performance*> test_cmd_;

  System::Semaphore sem_1_, sem_2_;

  static uint8_t static_mem_[64];

  static int Test(Performance* perf, int argc, char** argv) {
    XB_UNUSED(argc);
    XB_UNUSED(argv);

    printf("XRobot performance start running.\r\n");

    auto thread_test_fn = [](Performance* perf) {
      /* Semaphore */
      perf->sem_1_.Post();

      /* post and wait */
      perf->sem_2_.Wait();
      for (uint32_t i = 0; i < 500000; i++) {
        perf->sem_1_.Post();
        if (!perf->sem_2_.Wait(1000)) {
          OMLOG_ERROR("123 %d", i);
        }
      }
      System::Thread::Sleep(100);

      /* post */
      for (int i = 0; i < 1000000; i++) {
        perf->sem_1_.Post();
      }
      perf->sem_2_.Post();

      System::Thread::Sleep(100);
      perf->sem_1_.Post();
      uint64_t time = bsp_time_get();
      for (int i = 0; i < 1000000; i++) {
        perf->sem_1_.Post();
      }
      time = bsp_time_get() - time;

      printf("\tPost millon times\r\n");

      printf("\t\t%f microseconds per session\r\n",
             static_cast<float>(time) / 1000.0f / 1000000.0f);

      perf->sem_2_.Post();

      System::Thread::Sleep(UINT32_MAX);
    };

    perf->thread_test.Create(thread_test_fn, perf, "perf_thread", 1024,
                             System::Thread::MEDIUM);

    bool thread_support = perf->sem_1_.Wait(200);

    if (!thread_support) {
      printf("ERR:This device does not support multithreading.\r\n");
    } else {
      printf("*** Semaphore Test Start ***\r\n");
      /* post and wait */
      System::Thread::Sleep(200);
      perf->sem_2_.Post();
      auto time = bsp_time_get();

      for (uint32_t i = 0; i < 500000; i++) {
        perf->sem_1_.Wait(UINT32_MAX);
        perf->sem_2_.Post();
      }

      System::Thread::Sleep(200);

      time = bsp_time_get() - time;

      printf("\tWait and post millon times\r\n");

      printf("\t\t%f microseconds per session\r\n",
             static_cast<float>(time) / 1000.0f / 1000000.0f);

      /* post */
      perf->sem_2_.Wait();
      time = bsp_time_get();
      for (int i = 0; i < 1000000; i++) {
        if (!perf->sem_1_.Wait(1000)) {
          OMLOG_ERROR("%d", i);
        }
      }
      time = bsp_time_get() - time;

      printf("\tWait millon times\r\n");

      printf("\t\t%f microseconds per session\r\n",
             static_cast<float>(time) / 1000.0f / 1000000.0f);

      perf->sem_2_.Wait(UINT32_MAX);

      printf("*** Semaphore Test End ***\r\n\n");
    }

    perf->thread_test.Delete();

    printf("*** Memory Test Start ***\r\n");

    void* mem = malloc(1024);

    auto time = bsp_time_get();

    for (int i = 0; i < 10000; i++) {
      memset(mem, 0, 1024);
    }

    time = bsp_time_get() - time;

    printf("\tOverwrite 1k length memory in heap 10000 times\r\n");
    printf("\t\t%f Mb/s\r\n",
           10000.0 / static_cast<float>(time) / 1024.0 * 1000000);

    time = bsp_time_get();

    for (int i = 0; i < 10000 * 16; i++) {
      memset(mem, 0, 64);
    }

    time = bsp_time_get() - time;

    printf("\tOverwrite 64byte length memory in heap 160000 times\r\n");
    printf("\t\t%f Mb/s\r\n",
           10000.0 / static_cast<float>(time) / 1024.0 * 1000000);

    free(mem);

    time = bsp_time_get();

    for (int i = 0; i < 10000 * 16; i++) {
      memset(static_mem_, 0, 64);
    }

    time = bsp_time_get() - time;

    printf("\tOverwrite 64byte length memory in stack 160000 times\r\n");
    printf("\t\t%f Mb/s\r\n",
           10000.0 / static_cast<float>(time) / 1024.0 * 1000000);

    printf("*** Memory Test End ***\r\n");

    printf("*** Float Test Start ***\r\n");
    float a = 1.0;
    auto start_time = bsp_time_get();
    for (uint32_t i = 0; i < 1000000; i++) {
      a = a * static_cast<float>(i);
    }
    auto end_time = bsp_time_get();
    printf("Multiply floating-point numbers 1000 times use %d us\r\n",
           static_cast<uint32_t>((end_time - start_time) / 1000));
    printf("*** Float Test End ***\r\n");

    return 0;
  }

  Performance() : test_cmd_(this, Test, "perf"), sem_1_(0), sem_2_(0) {}
};
}  // namespace Module
