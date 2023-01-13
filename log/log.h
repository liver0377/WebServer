#ifndef LOG_H
#define LOG_H

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include "log/block_queue.h"

#define DIR_NAME_SIZE 128
#define FILE_NAME_SIZE 128
#define LEVEL_NAME_SIZE 10
#define TIME_MESSAGE_SIZE 30

class Log {
  public:
   /**
    * @brief 获取唯一Log实例, 内部使用的是static局部对象
    *
    * @return Log*
    */
   static Log* get_instance() {
      static Log instance;
      return &instance;
   }

   /**
    * @brief 初始化Log实例: 设置类内变量, 打开日志文件, [创建异步线程],
    *
    * @param filename       Log文件名称, 真实文件名称会以此为基础
    * @param close_log      是否关闭Log
    * @param log_buf_size   Log缓冲区的大小, 默认为8192
    * @param split_lines    单个Log文件的最大行数, 默认为500000
    * @param max_queue_size 阻塞队列大小, 默认为0
    * @return true
    * @return false
    */
   bool init(const char* filename, int close_log, int log_buf_size = 8192,
             int split_lines = 500000, int max_queue_size = 0);

   /**
    * @brief 将日志从阻塞队列刷新到日志文件
    *        被线程所调用
    *
    * @param args
    * @return void*
    */
   static void* flush_log_thread(void* args) {

      Log::get_instance()->async_write_log();

      return (void*)0;
   }

   /**
    * @brief 将指定格式的日志字符串写入日志文件
    * @details 若指定了m_async异步标志, 那么会将工作委托给异步写线程,
    * 否则会同步写入日志文件
    *
    * @param level  日志等级, 包括: DEBUG(0), INFO(1), WARR(2), ERROR(3)
    * @param format  日志格式
    * @param ...  可选参数
    */
   void write_log(int level, const char* format, ...);

   /**
    * @brief 将阻塞队列的内容刷新到日志文件
    *
    */
   void flush(void);

  private:
   /**
    * @brief 构建对象
    *
    */
   Log();

   /**
    * @brief 在程序结束时释放对象
    *
    */
   ~Log();

   /**
    * @brief 从阻塞队列中取出一个日志字符串, 将其写入日志文件
    *
    * @return void*
    */
   void* async_write_log() {
      std::string log_string;

      while (m_log_queue->pop(log_string)) {
         m_mutex.lock();

         fputs(log_string.c_str(), m_fp);

         m_mutex.unlock();
      }

      return (void*)0;
   }

  private:
   char dir_name[DIR_NAME_SIZE];   // 日志路径名称
   char log_name[FILE_NAME_SIZE];  // 日志文件名称

   int m_split_lines;   // 单个日志文件的最大行数
   int m_log_buf_size;  // 日志缓冲区大小
   int m_close_log;     // 关闭日志
   int m_today;         // 当前天数

   long long m_count;  // 今日已经记录的日志文件的行数
   bool m_is_async;    // 是否同步的标志位
   locker m_mutex;     // 日志文件的mutex
   pthread_t m_tid;

   char* m_buf;  // 日志缓冲区, 用户缓冲单行日志
   FILE* m_fp;   // Log文件指针
   block_queue<std::string>* m_log_queue;  // 阻塞队列
};

#define LOG_DEBUG(format, ...)                                  \
   if (0 == m_close_log) {                                      \
      Log::get_instance()->write_log(0, format, ##__VA_ARGS__); \
      Log::get_instance()->flush();                             \
   }
#define LOG_INFO(format, ...)                                   \
   if (0 == m_close_log) {                                      \
      Log::get_instance()->write_log(1, format, ##__VA_ARGS__); \
      Log::get_instance()->flush();                             \
   }
#define LOG_WARR(format, ...)                                   \
   if (0 == m_close_log) {                                      \
      Log::get_instance()->write_log(2, format, ##__VA_ARGS__); \
      Log::get_instance()->flush();                             \
   }
#define LOG_ERROR(format, ...)                                  \
   if (0 == m_close_log) {                                      \
      Log::get_instance()->write_log(3, format, ##__VA_ARGS__); \
      Log::get_instance()->flush();                             \
   }
#endif
