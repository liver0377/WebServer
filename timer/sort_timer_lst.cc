#include "timer/sort_timer_lst.h"
#include "http/http_conn.h"
#include "utils/utils.h"

sort_timer_lst::sort_timer_lst() : head(NULL), tail(NULL) {}

sort_timer_lst::~sort_timer_lst() {
   util_timer* cur = head;
   while (cur != NULL) {
      util_timer* nxt = cur->next;
      delete cur;
      cur = nxt;
   }
}

void sort_timer_lst::add_timer(util_timer* timer) {
   if (timer == NULL) {
      return;
   }

   if (head == NULL) {
      head = tail = timer;
      return;
   }

   if (timer->expire < head->expire) {
      timer->next = head;
      head->prev = timer;
      head = timer;
      return;
   }

   add_timer(timer, head);
}

void sort_timer_lst::add_timer(util_timer* timer, util_timer* lst_head) {
   util_timer* prev = lst_head;
   util_timer* tmp = prev->next;

   while (tmp) {
      if (timer->expire < tmp->expire) {
         prev->next = timer;
         timer->next = tmp;
         tmp->prev = timer;
         timer->prev = prev;
         break;
      }
      prev = tmp;
      tmp = tmp->next;
   }

   if (!tmp) {
      prev->next = timer;
      timer->prev = prev;
      timer->next = NULL;
      tail = timer;
   }
}

void sort_timer_lst::adjust_timer(util_timer* timer) {
   if (timer == NULL) {
      return;
   }

   util_timer* t = timer->next;
   if (t == NULL || timer->expire < t->expire) {
      return;
   }

   if (head == timer) {
      head = head->next;
      head->prev = NULL;
      timer->next = NULL;
      add_timer(timer, head);
   } else {
      timer->prev->next = timer->next;
      timer->next->prev = timer->prev;
      add_timer(timer, timer->next);
   }
}

void sort_timer_lst::del_timer(util_timer* timer) {
   if (timer == NULL) {
      return;
   }

   if (timer == head && timer == tail) {
      delete timer;
      head = NULL;
      tail = NULL;
      return;
   }

   if (timer == head) {
      head = head->next;
      head->prev = NULL;
      delete timer;
      return;
   }

   if (timer == tail) {
      tail = tail->prev;
      tail->next = NULL;
      delete timer;
      return;
   }

   timer->prev->next = timer->next;
   timer->next->prev = timer->prev;
   delete timer;
}

void sort_timer_lst::tick() {
   if (head == NULL) {
      return;
   }

   time_t cur_time = time(NULL);
   util_timer* cur = head;
   while (cur != NULL) {
      if (cur->expire > cur_time) {
         break;
      }

      cur->cb_func(cur->data);

      head = cur->next;
      if (head != NULL) {
         head->prev = NULL;
      } else {
         tail = NULL;
      }
      delete cur;
      cur = head;
   }
}

// extern int Utils::m_epollfd;
// extern int http_conn::m_user_count;
/**
 * @brief 定时器超时时的回调函数
 *
 * @param user_data  定时器所对应的连接的用户数据
 */
void cb_func(client_data* user_data) {
   assert(epoll_ctl(Utils::m_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0) !=
          -1);
   assert(user_data);
   close(user_data->sockfd);
   http_conn::m_user_count--;
}