#ifndef SORT_TIMER_LST_H
#define SORT_TIMER_LST_H

#include <netinet/in.h>
#include <time.h>


class util_timer;

struct client_data {
   sockaddr_in client_address;  // 客户IPV4套接字地址
   int sockfd;                  // 连接套接字描述符
   util_timer* timer;           // 客户连接对应的定时器
};

class util_timer {
  public:
   util_timer() : prev(NULL), next(NULL) {}
   ~util_timer() {}

  public:
   time_t expire;                  // 计时器过期时间
   void (*cb_func)(client_data*);  // 回调函数, 计时器超时时调用
   client_data* data;              // 回调函数参数
   util_timer* prev;
   util_timer* next;  // 前向指针, 后向指针
};

/**
 * @brief 定时器链表, 是一个双向链表
 *
 */
class sort_timer_lst {
  public:
   sort_timer_lst();
   ~sort_timer_lst();

   /**
    * @brief 将指定定时器添加到定时器链表
    *
    * @param timer
    */
   void add_timer(util_timer* timer);

   /**
    * @brief 当定时器重置时, 调整其在定时器链表中的位置
    *
    * @param timer
    */
   void adjust_timer(util_timer* timer);

   /**
    * @brief 从定时器链表中删除一个指定定时器
    *
    * @param timer
    */
   void del_timer(util_timer* timer);

   /**
    * @brief 心搏函数, 每隔TIME_SLOT时间就会触发一次,
    * 用于清理定时器链表中的超时定时器
    *
    */
   void tick();

  private:
   /**
    * @brief 将timer插入到head为首的链表中, head保证有效, 并且保证不会发生头插
    *
    * @param timer
    * @param head
    */
   void add_timer(util_timer* timer, util_timer* head);

  private:
   util_timer* head;
   util_timer* tail;
};


#endif