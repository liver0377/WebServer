#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <list>
#include "http/http_conn.h"
#include "locker/locker.h"
#include "log/log.h"

template <typename T>
class ThreadPool {
  public:
   ThreadPool(int actor_model, connection_pool* conn_pool,
              int thread_number = 8, int max_request = 10000);
   ~ThreadPool();
   bool append(T* request, int state);
   bool append_p(T* request);

  private:
   void* worker(void* arg);
   void run();

  private:
   int m_thread_num;              // 线程池中的线程数目
   int m_max_request;             // 线程池允许的最大请求数目
   pthread_t* m_threads;          // 线程数组
   std::list<T*> m_work_queue;    // 请求队列
   sem m_queuestate;              // 请求队列信号量
   locker m_queuelocker;          // 请求队列的锁
   connection_pool* m_conn_pool;  // 数据库连接池
   int m_actor_model;             // 模型切换
};

template <typename T>
ThreadPool<T>::ThreadPool(int actor_model, connection_pool* conn_pool,
                          int thread_num, int max_request) {
   if (thread_num <= 0 || max_request <= 0) {
      throw std::exception();
   }

   m_threads = new pthread_t[thread_num];
   if (!m_threads) {
      throw std::exception();
   }

   for (int i = 0; i < thread_num; i++) {
      if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
         delete[] m_threads;
         throw std::exception();
      }

      if (pthread_detach(m_threads[i]) != 0) {
         delete[] m_threads;
         throw std::exception();
      }
   }
}

template <typename T>
ThreadPool<T>::~ThreadPool() {
   delete[] m_threads;
}

template <typename T>
bool ThreadPool<T>::append(T* request, int state) {
   m_queuelocker.lock();

   if (m_work_queue.size() >= m_max_request) {
      m_queulocker.unlock();
      return false;
   }

   // ??
   request->m_state = state;

   m_work_queue->push_back(request);
   m_queuelocker.unlock();
   m_queuestate.post();
   return true;
}

template <typename T>
bool ThreadPool<T>::append_p(T* request) {
   m_queuelocker.lock();

   if (m_work_queue.size() >= m_max_request) {
      m_queulocker.unlock();
      return false;
   }

   m_work_queue->push_back(request);
   m_queuelocker.unlock();
   m_queuestate.post();
   return true;
}

template <typename T>
void* ThreadPool<T>::worker(void* args) {
   ThreadPool* p = (ThreadPool*)args;
   p->run();
   return pool;
}

template <typename T>
void ThreadPool<T>::run() {
   while (true) {
      m_queuestate.wait();
      m_queuelocker.lock();

      if (m_work_queue.empty()) {
         m_queuelocker.unlock();
         continue;
      }

      T* request = m_work_queue.front();
      m_work_queue.pop_front();
      m_queuelocker.unlock();
      if (!request) {
         continue
      };

      if (1 == m_actor_model) {
         // Reactor模式
         if (0 == request->m_state) {
            if (request->read()) {
               request->improv = 1;
               connectionRAII mysqlcon(&request->mysql, m_connPool);
               request->process();
            } else {
               request->improv = 1;
               request->timer_flag = 1;
            }
         } else {
            if (request->write()) {
               request->improv = 1;
            } else {
               request->improv = 1;
               request->timer_flag = 1;
            }
         }
      } else {
         // Proactor模式
         connectionRAII mysqlcon(&request->mysql, m_connPool);
         request->process();
      }
   }
}
#endif