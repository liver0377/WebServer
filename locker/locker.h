#ifndef LOCKER_H
#define LOCKER_H

#include <pthread.h>
#include <semaphore.h>
#include <exception>

/**
 * @brief sem 类是对Posix 信号量操作的封装
 *         其使用了RAII, 在构造函数中创建未命名信号量
 *         在析构函数中将未命名信号量销毁
 */
class sem {
  public:
      sem() {
        if (sem_init(&m_sem, 0, 0) == -1) {
            throw std::exception();
        }
      }

      sem(int num) {
        if (sem_init(&m_sem, 0, num) == -1) {
            throw std::exception();
        }
      }

      ~sem() {
        sem_destroy(&m_sem);
      }

      bool wait() {
          return sem_wait(&m_sem) == 0;
      }

      bool post() {
        return sem_post(&m_sem) == 0;
      }

      bool try_wait() {
        return sem_trywait(&m_sem) == 0;
      }

  private:
   sem_t m_sem;
};


/**
 * @brief locker 类是对mutex操作的封装
 *        其使用了RAII机制, 在构造函数中创建mutex
 *        在析构函数中释放mutex
 * 
 */
class locker {
    public:
    locker() {
        if (pthread_mutex_init(&m_mutex, NULL) != 0) {
            throw std::exception();
        }
    }

    ~locker() {
        pthread_mutex_destroy(&m_mutex);
    }

    bool lock() {
        return pthread_mutex_lock(&m_mutex) == 0;
    }

    bool unlock() {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

    bool trylock() {
        return pthread_mutex_trylock(&m_mutex) == 0;
    }

    pthread_mutex_t *get() {
        return &m_mutex;
    }

    private:
    pthread_mutex_t m_mutex;
};


/**
 * @brief cond 类是对条件变量类的封装
 *        其使用了RAII, 在构造函数中初始化条件变量
 *        在析构函数中销毁条件变量
 */
class cond {
    public:
    cond() {
        if (pthread_cond_init(&m_cond, NULL) != 0) {
            throw std::exception();
        }
    }    

    ~cond() {
        pthread_cond_destroy(&m_cond);
    }

    bool wait(pthread_mutex_t *mutex) {
        return pthread_cond_wait(&m_cond, mutex) == 0;
    }

    bool timewait(pthread_mutex_t *mutex, struct timespec *abstime) {
        return pthread_cond_timedwait(&m_cond, mutex, abstime) == 0;
    }

    bool signal() {
        return pthread_cond_signal(&m_cond) == 0; 
    }

    bool broadcast() {
        return pthread_cond_broadcast(&m_cond) == 0;
    }

    private:
    pthread_cond_t m_cond;
};
#endif
