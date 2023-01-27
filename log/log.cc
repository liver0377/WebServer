#include "log/log.h"

#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

Log::Log()
    : m_split_lines(500000),
      m_log_buf_size(8192),
      m_close_log(false),
      m_count(0),
      m_is_async(false),
      m_tid(0),
      m_buf(NULL),
      m_fp(NULL) {}

Log::~Log() {
    if (m_buf != NULL) {
      delete[] m_buf;
    }

  if (m_fp != NULL) {
    fclose(m_fp);
  }

  if (m_tid != 0) {
    m_log_queue->m_child_exit = 1;
    // m_log_queue->push("meaningless");
    while (m_log_queue->m_child_exit != 2) {
      m_log_queue->push("meaningless");
      sleep(1);
    }

    pthread_join(m_tid, NULL);
  }

  if (m_log_queue != NULL) {
    delete m_log_queue;
  }
}

bool Log::init(const char* filename, int close_log, int log_buf_size,
               int split_lines, int max_queue_size) {
  std::cout << "Log::init()" << std::endl;
  // 表明启用异步线程

  // 目录名, 文件名的处理
  char log_full_name[DIR_NAME_SIZE + FILE_NAME_SIZE + TIME_MESSAGE_SIZE];

  memset(log_full_name, 0, sizeof(log_full_name));
  memset(dir_name, 0, sizeof(dir_name));
  memset(log_name, 0, sizeof(log_name));

  const char* p = basename(filename);
  strcpy(log_name, p);

  time_t t = time(NULL);
  struct tm* sys_tm =
      localtime(&t);  // localtime为不可重入函数, 需要自己手动拷贝一份
  struct tm my_tm = *sys_tm;

  p = strrchr(filename, '/');
  if (p == NULL) {
    snprintf(log_full_name, sizeof(log_full_name), "%d_%02d_%02d_%s",
             my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, filename);
  } else {
    strncpy(dir_name, filename, p - filename + 1);
    snprintf(log_full_name, sizeof(log_full_name), "%s%d_%02d_%02d_%s",
             dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
             log_name);
  }

  m_split_lines = split_lines;
  m_log_buf_size = log_buf_size;
  m_close_log = close_log;
  m_today = my_tm.tm_mday;
  m_buf = new char[log_buf_size];

  m_fp = fopen(log_full_name, "a");

  if (m_fp == NULL) {
    return false;
  }

  if (max_queue_size > 0) {
    std::cout << "use async log thread" << std::endl;
    m_is_async = true;
    m_log_queue = new block_queue<std::string>(max_queue_size);
    pthread_create(&m_tid, NULL, flush_log_thread, NULL);
  }

  return true;
}

void Log::write_log(int level, const char* format, ...) {
  std::cout << "write_log()" << std::endl;
  struct timeval now;

  gettimeofday(&now, NULL);
  time_t t = now.tv_sec;
  struct tm* sys_time = localtime(&t);
  struct tm my_tm = *sys_time;

  // 保证对m_today, m_count, m_fp的互斥访问
  m_mutex.lock();
  m_count++;

  std::cout << "109" << std::endl;
  // 创建新文件
  if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0) {
    char new_log[FILE_NAME_SIZE * 2 + DIR_NAME_SIZE + TIME_MESSAGE_SIZE + 20] =
        {0};
    fflush(m_fp);
    fclose(m_fp);

    char tail[FILE_NAME_SIZE + TIME_MESSAGE_SIZE] = {0};

    snprintf(tail, sizeof(tail), "%d_%02d_%02d_%s", my_tm.tm_year, my_tm.tm_mon,
             my_tm.tm_mday, log_name);

    if (m_today != my_tm.tm_mday) {
      snprintf(new_log, sizeof(new_log), "%s%s%s", dir_name, tail, log_name);
      m_today = my_tm.tm_mday;
      m_count = 0;
    } else {
      // 同一天可能会产生多个日志文件, 为了加以区分,
      // 需要使用不同的日志文件名字
      snprintf(new_log, sizeof(new_log), "%s%s%s.%lld", dir_name, tail,
               log_name, m_count / m_split_lines);
    }
    m_fp = fopen(new_log, "a");
  }

  m_mutex.unlock();

  std::cout << "137" << std::endl;
  char level_str[LEVEL_NAME_SIZE] = {0};
  switch (level) {
    case 0:
      strcpy(level_str, "[debug]");
      break;

    case 1:
      strcpy(level_str, "[info]");
      break;

    case 2:
      strcpy(level_str, "[warring]");
      break;

    case 3:
      strcpy(level_str, "[error]");
      break;

    default:
      strcpy(level_str, "[unknown]");
      break;
  }

  va_list ap;
  va_start(ap, format);

  m_mutex.lock();

  std::cout << "165" << std::endl;
  int n = snprintf(m_buf, m_log_buf_size,
                   "%d-%02d-%02d %02d:%02d:%02d.%06ld %s", my_tm.tm_year + 1900,
                   my_tm.tm_mon + 1, my_tm.tm_mday, my_tm.tm_hour, my_tm.tm_min,
                   my_tm.tm_sec, now.tv_usec, level_str);
  int m = vsnprintf(m_buf + n + 1, m_log_buf_size, format, ap);

  m_buf[n] = ' ';
  m_buf[m + n + 1] = '\n';
  m_buf[m + n + 2] = '\0';
  std::cout << "176" << std::endl;

  std::string log_str(m_buf);
  std::cout << "179" << std::endl;
  m_mutex.unlock();

  if (m_is_async && !m_log_queue->full()) {
    std::cout << "181" << std::endl;
    if (m_log_queue == NULL) {
      throw std::exception();
    }
    m_log_queue->push(log_str);
    std::cout << "183" << std::endl;
  } else {
    m_mutex.lock();

    std::cout << "186" << std::endl;
    fputs(m_buf, m_fp);
    std::cout << "187" << std::endl;

    m_mutex.unlock();
  }

  std::cout << "190" << std::endl;
  va_end(ap);

  std::cout << "write_log() over" << std::endl;
}

void Log::flush() {
  m_mutex.lock();

  fflush(m_fp);

  m_mutex.unlock();
}
