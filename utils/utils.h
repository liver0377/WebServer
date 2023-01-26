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
#include <iostream>

class Utils {
 public:
  static void init(int epollfd, int* sig_pipe, int timeslot);

  static int setnoblocking(int fd);

  static void addfd(int epollfd, int fd, bool enable_oneshot, bool enable_et,
                    bool enable_out = false);

  static void removefd(int epollfd, int fd);

  static void modifyfd(int epollfd, int fd, int event, bool enable_et);

  static void sig_handler(int sig);

  static void addsig(int sig, void (*handler)(int sig), bool restart = true);

 public:
  static int m_epollfd;    // 整个进程的内核事件表句
  static int* m_sig_pipe;  // 信号处理器与整个进程的管道
  static int m_TIMESLOT;   // TIME_SLOT
};

#endif