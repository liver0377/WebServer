#include "utils/utils.h"

int* Utils::m_sig_pipe = NULL;
int Utils::m_TIMESLOT = 1;
int Utils::m_epollfd = -1;

/**
 * @brief 初始化Utils类
 *
 * @param sig_pipe   整个进程的信号管道地址
 * @param timeslot   指定TIME_SLOT的值
 */
void Utils::init(int epollfd, int* sig_pipe, int timeslot) {
   epollfd = m_epollfd;
   m_sig_pipe = sig_pipe;
   m_TIMESLOT = timeslot;
}

/**
 * @brief 为指定文件描述符设置O_NONBLOCK标志
 *
 * @param fd 待设置的文件描述符
 * @return int fd上旧的标志
 */
int Utils::setnoblocking(int fd) {
   int old_option = fcntl(fd, F_GETFD);
   int new_option = old_option | O_NONBLOCK;
   fcntl(fd, F_SETFD, new_option);

   return old_option;
}

/**
 * @brief  将指定文件描述符注册到内核事件表
 *
 * @param epollfd  内核事件表描述符
 * @param fd       待监听的文件描述符
 * @param enable_oneshot 是否启用EPOLLONESHOT
 * @param enable_et 是否启用EPOLLET
 *                 默认的触发事件为EPOLLIN | EPOLLHUP
 */
void Utils::addfd(int epollfd, int fd, bool enable_oneshot, bool enable_et) {
   epoll_event event;
   event.data.fd = fd;

   int events = EPOLLIN | EPOLLHUP;
   if (enable_oneshot) {
      events |= EPOLLONESHOT;
   }
   if (enable_et) {
      events |= EPOLLET;
   }
   event.events = events;

   assert(epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event) != -1);
   setnoblocking(fd);
}

/**
 * @brief 信号处理器函数
 *
 * @param sig  触发该函数的信号值
 */
void Utils::sig_handler(int sig) {
   int saved_errno = errno;

   int msg = sig;
   send(m_sig_pipe[1], (char*)&msg, 1, 0);

   errno = saved_errno;
}

/**
 * @brief 为当前进程添加信号处理函数
 *
 * @param sig  添加信号sig的信号处理函数
 * @param handler 信号处理函数
 * @param restart 是否启用SA_RESTART标志
 */
void Utils::addsig(int sig, void (*handler)(int sig), bool restart) {
   struct sigaction act;

   act.sa_handler = handler;
   sigfillset(&act.sa_mask);

   if (restart) {
      act.sa_flags |= SA_RESTART;
   }

   assert(sigaction(sig, &act, NULL) != -1);
}

void Utils::removefd(int epollfd, int fd) {
   assert(epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) != -1);
   close(fd);
}

void Utils::modifyfd(int epollfd, int fd, int event, bool enable_et) {
   epoll_event ev;
   ev.data.fd = fd;

   int events = EPOLLIN | EPOLLHUP | EPOLLONESHOT;
   if (enable_et) {
      events |= EPOLLET;
   }

   ev.events = events;
   assert(epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev) != -1);
}


