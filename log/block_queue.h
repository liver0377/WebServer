#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
#include "locker/locker.h"

template <typename T>
class block_queue {
  public:
   /**
    * @brief 构造一个新的阻塞队列
    *
    * @param max_size 队列的最大容量, 默认为1000
    */
   block_queue(int max_size = 1000)
       : m_child_exit(0),
         m_size(0),
         m_max_size(max_size),
         m_front(0),
         m_back(0) {

      if (max_size <= 0) {
         exit(-1);
      }
      m_array = new T[max_size];
   }

   ~block_queue() {
      m_mutex.lock();

      if (m_array != NULL) {
         delete[] m_array;
      }

      m_mutex.unlock();
   }

   /**
    * @brief 释放内存空间之前的预处理工作
    *
    */
   void clear() {
      m_mutex.lock();

      m_size = 0;
      m_max_size = 0;
      m_front = 0;
      m_back = 0;

      m_mutex.unlock();
   }

   /**
    * @brief 判断队列是否满了
    *
    * @return true
    * @return false
    */
   bool full() {
      m_mutex.lock();

      if (m_size >= m_max_size) {
         m_mutex.unlock();
         return true;
      }

      m_mutex.unlock();
      return false;
   }

   /**
    * @brief 判断队列是否为空
    *
    * @return true
    * @return false
    */
   bool empty() {
      m_mutex.lock();

      if (m_size == 0) {
         m_mutex.unlock();
         return true;
      }

      m_mutex.unlock();
      return false;
   }

   /**
    * @brief 返回队首元素
    *
    * @param[out] item 通过引用返回队首元素
    * @return true
    * @return false
    */
   bool front(T& item) {
      m_mutex.lock();

      if (m_size == 0) {
         m_mutex.unlock();
         return false;
      }

      item = m_array[m_front];

      m_mutex.unlock();
      return true;
   }

   /**
    * @brief 返回队尾元素
    *
    * @param[out] item 通过引用返回队尾元素
    * @return true
    * @return false
    */
   bool back(T& item) {
      m_mutex.lock();

      if (m_size == 0) {
         m_mutex.unlock();
         return false;
      }

      item = m_array[m_back];

      m_mutex.unlock();
      return true;
   }

   /**
    * @brief 返回阻塞队列中的元素个数
    *
    * @return int
    */
   int size() {
      m_mutex.lock();

      int t = m_size;

      m_mutex.unlock();
      return t;
   }

   /**
    * @brief 返回阻塞队列的容量
    *
    * @return int
    */
   int max_size() {
      m_mutex.lock();

      int t = m_max_size;

      m_mutex.unlock();
      return t;
   }

   /**
    * @brief 向阻塞队列尾部添加一个元素
    *        当队列满的时候, 会直接返回, 并非阻塞操作
    * @param item
    * @return true
    * @return false
    */
   bool push(const T& item) {
      m_mutex.lock();

      if (m_size >= m_max_size) {
         m_cond.broadcast();
         m_mutex.unlock();
         return false;
      }

      m_array[m_back] = item;
      m_back = (m_back + 1) % m_max_size;
      m_size++;

      m_cond.broadcast();
      m_mutex.unlock();
      return true;
   }

   /**
    * @brief 从阻塞队列首部弹出一个元素
    *        当队列空的时候, 会陷入阻塞
    * @return true
    * @return false
    */
   bool pop(T& item) {
      m_mutex.lock();

      while (m_size <= 0) {
         if (!m_cond.wait(m_mutex.get())) {
            m_mutex.unlock();
            return false;
         }

         if (m_child_exit == 1) {
            m_child_exit = 2;
            m_mutex.unlock();
            pthread_exit(NULL);
         }
      }

      item = m_array[m_front];
      m_front = (m_front + 1) % m_max_size;
      m_size--;

      m_mutex.unlock();
      return true;
   }

   /**
    * @brief 从阻塞队列队首弹出一个元素
    *        当队列为空时, 会陷入阻塞
    *
    * @param[out] item  以引用返回队首元素
    * @param ms_timeout 超时时间, 单位为毫秒
    * @return true
    * @return false
    */
   bool pop(T& item, int ms_timeout) {
      m_mutex.lock();

      struct timespec t = {0, 0};
      struct timeval now = {0, 0};

      gettimeofday(&now, NULL);
      t.tv_sec = now.tv_sec + ms_timeout / 1000;
      t.tv_nsec = 0;

      if (m_size == 0) {
         if (!m_cond.timewait(m_mutex.get(), &t)) {
            m_mutex.unlock();
            return false;
         }

         if (m_child_exit == 1) {
            m_child_exit = 2;
            m_mutex.unlock();
            pthread_exit(NULL);
         }
      }

      if (m_size == 0) {
         m_mutex.unlock();
         return false;
      }

      item = m_array[m_front];
      m_front = (m_front + 1) % m_max_size;
      m_size--;

      m_cond.broadcast();
      m_mutex.unlock();
      return true;
   }

  public:
   int m_child_exit;

  private:
   locker m_mutex;  // 保证互斥操作
   cond m_cond;     // 生产者消费者共用一个条件变量

   T* m_array;      // 队列
   int m_size;      // 队列当前元素个数
   int m_max_size;  // 队列容量
   int m_front;     // 队列首
   int m_back;      // 队列尾
};
#endif