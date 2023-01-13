#ifndef UTILS_H
#define UTILS_H

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cassert>
#include <cstring>

class Utils {
  public:
   static int setnoblocking(int fd);

   static void addfd(int epollfd, int fd, bool enable_oneshot, bool enable_et);

   static void removefd(int epollfd, int fd);

   static void modifyfd(int epollfd, int fd, int event, bool enable_et);

   static void sig_handler(int sig);

   static void addsig(int sig, void (*handler)(int sig), bool restart = true);

  public:
   static int m_sig_pipefd[2];  // 信号处理器与整个进程的管道
   static int m_TIMESLOT;       // TIME_SLOT
};

#endif