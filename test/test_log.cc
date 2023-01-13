#include "gtest/gtest.h"
#include "log/block_queue.h"
#include "log/log.h"

TEST(TestLog, test_block_queue_push) {
   block_queue<int> q;
   q.push(10);

   int ret = -1;
   q.front(ret);
   EXPECT_EQ(10, ret);
}

TEST(TestLog, test_block_queue_pop) {
   block_queue<int> q(10);
   q.push(1);
   q.push(2);
   q.push(3);

   int ret = -1;
   q.pop(ret);
   EXPECT_EQ(1, ret);

   q.pop(ret);
   EXPECT_EQ(2, ret);

   q.pop(ret);
   EXPECT_EQ(3, ret);
}

TEST(TestLog, test_block_queue_circle) {
   block_queue<int> q(100);
   for (int i = 0; i < 80; i++) {
      q.push(i);
   }

   for (int i = 0; i < 20; i++) {
      int ret;
      q.pop(ret);
   }

   for (int i = 0; i < 40; i++) {
      q.push(i);
   }

   EXPECT_EQ(true, q.full());
}

TEST(TestLog, test_block_queue_pop_is_block) {
   block_queue<int> q;
   int item;
   q.pop(item, 2000);
}

TEST(TestLog, test_log_sync) {
    Log *log = Log::get_instance();
    log->init("web_srv.log", 0, 8192, 500000, 0);

    log->write_log(0, "%s", "test sync log writing");
}

TEST(TestLog, test_log_async) {
   Log *log = Log::get_instance();
   log->init("web_srv.log", 0, 8192, 500000, 1000);

   sleep(1);

   log->write_log(1, "%s", "test async log writing");
}