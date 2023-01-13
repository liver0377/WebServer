#include "gtest/gtest.h"
#include "timer/sort_timer_lst.h"

TEST(test_timer, test_sort_timer_lst_random_add_delete) {
   sort_timer_lst list;
   util_timer* timers[100];
   for (int i = 0; i < 100; i++) {
      timers[i] = new util_timer;
   }

   srand((unsigned int)time(NULL));
   for (int i = 0; i < 100; i++) {
      timers[i]->expire = random() % 1000;
   }

   for (int i = 0; i < 100; i++) {
      list.add_timer(timers[i]);
   }

   for (int i = 0; i < 100; i++) {
      list.del_timer(timers[i]);
   }
}

TEST(test_timer, test_sort_timer_lst_head_add) {
   sort_timer_lst list;
   util_timer* timers[100];
   for (int i = 0; i < 100; i++) {
      timers[i] = new util_timer;
      timers[i]->expire = i;
   }

   for (int i = 0; i < 100; i++) {
      list.add_timer(timers[i]);
   }

   for (int i = 0; i < 100; i++) {
      list.del_timer(timers[i]);
   }
}

TEST(test_timer, test_sort_timer_lst_tail_add) {
   sort_timer_lst list;
   util_timer* timers[100];
   for (int i = 0; i < 100; i++) {
      timers[i] = new util_timer;
      timers[i]->expire = 100 - i;
   }

   for (int i = 0; i < 100; i++) {
      list.add_timer(timers[i]);
   }

   for (int i = 0; i < 100; i++) {
      list.del_timer(timers[i]);
   }
}

static int cnt = 0;
void cb_func(client_data* data) { cnt++; }

// TEST(test_timer, test_sort_timer_lst_tick_1) {
//    sort_timer_lst list;
//    util_timer* timers[3] = {new util_timer, new util_timer, new util_timer};
//  
//    time_t cur_time = time(NULL);
//    timers[0]->expire = cur_time + 1;
//    timers[0]->cb_func = cb_func;
//    timers[1]->expire = cur_time + 3;
//    timers[1]->cb_func = cb_func;
//    timers[2]->expire = cur_time + 5;
//    timers[2]->cb_func = cb_func;
// 
//    list.add_timer(timers[0]);
//    list.add_timer(timers[1]);
//    list.add_timer(timers[2]);
// 
//    cnt = 0;
// 
//    sleep(2);
//    list.tick();
//    EXPECT_EQ(1, cnt);
// 
//    sleep(2);
//    list.tick();
//    EXPECT_EQ(2, cnt);
// 
//    sleep(2);
//    list.tick();
//    EXPECT_EQ(3, cnt);
// 
// }
// 
// TEST(test_timer, test_sort_timer_lst_tick_2) {
//    sort_timer_lst list;
//    util_timer* timers[3] = {new util_timer, new util_timer, new util_timer};
// 
//    time_t cur_time = time(NULL);
//    timers[0]->expire = cur_time + 1;
//    timers[0]->cb_func = cb_func;
//    timers[1]->expire = cur_time + 1;
//    timers[1]->cb_func = cb_func;
//    timers[2]->expire = cur_time + 3;
//    timers[2]->cb_func = cb_func;
// 
//    list.add_timer(timers[0]);
//    list.add_timer(timers[1]);
//    list.add_timer(timers[2]);
// 
//    cnt = 0;
// 
//    sleep(2);
//    list.tick();
//    EXPECT_EQ(2, cnt);
// 
//    sleep(2);
//    list.tick();
//    EXPECT_EQ(3, cnt);
// 
//    list.tick();
//    EXPECT_EQ(3, cnt);
// }