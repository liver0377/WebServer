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
  m_epollfd = epollfd;
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
  // std::cout << "set " << fd << "to nonblocking" << std::endl;
  int old_option = fcntl(fd, F_GETFL);
  int new_option = old_option | O_NONBLOCK;
  assert(fcntl(fd, F_SETFL, new_option) != -1);

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
 * @param enable)out 是否启用EPOLLOUT
 */
void Utils::addfd(int epollfd, int fd, bool enable_oneshot, bool enable_et,
                  bool enable_out) {
  // std::cout << "add " << fd << " to epollfd" << std::endl;
  epoll_event event;
  event.data.fd = fd;

  int events = EPOLLIN | EPOLLHUP;
  if (enable_oneshot) {
    events |= EPOLLONESHOT;
  }
  if (enable_et) {
    events |= EPOLLET;
  }
  if (enable_out) {
    events |= EPOLLOUT;
  }
  event.events = events;

  setnoblocking(fd);
  assert(epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event) != -1);
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
  // std::cout << "modifyfd()" << std::endl;
  epoll_event ev;
  ev.data.fd = fd;

  // 使用EPOLLONESHOT避免该socket被多个线程竞争处理
  ev.events = EPOLLHUP | EPOLLONESHOT;
  if (enable_et) {
    ev.events |= EPOLLET;
  }
  ev.events |= event;

  // std::cout << "m_epollfd: " << m_epollfd << std::endl;
  // std::cout << "m_connfd: " << fd << std::endl;
  assert(epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev) != -1); 
}
