#include "server/server.h"

WebServer::WebServer()
    : m_users(new http_conn[MAX_USER_COUNT]),
      m_users_timer(new client_data[MAX_USER_COUNT]) {
  if (m_users == NULL || m_users_timer == NULL) {
    LOG_ERROR("not enough memory");
    throw std::exception();
  }
  char server_root[200];
  if (getcwd(server_root, 200) == NULL) {
    throw std::exception();
  }

  memset(m_root, '\0', 1024);
  strcpy(m_root, server_root);

  char source_root[] = "/root";
  strcat(m_root, source_root);
}

WebServer::~WebServer() {
  close(m_epollfd);
  close(m_listenfd);
  delete[] m_users;
  delete[] m_users_timer;
  delete m_thread_pool;
  delete m_timer_lst;
}

void WebServer::init(int port, const std::string& user,
                     const std::string& password, const std::string& db_name,
                     int log_mode, int opt_linger, int trigger_mode,
                     int connection_pool_size, int thread_num, int close_log,
                     int actor_model) {
  m_port = port;
  m_user = user;
  m_password = password;
  m_db_name = db_name;
  m_log_mode = log_mode;
  m_opt_linger = opt_linger;
  m_trigger_mode = trigger_mode;
  m_connection_pool_size = connection_pool_size;
  m_thread_num = thread_num;
  m_close_log = close_log;
  m_actor_model = actor_model;
}

void WebServer::timerLst() { m_timer_lst = new sort_timer_lst; }
/**
 * @brief 初始化线程池
 *
 */
void WebServer::threadPool() {
  m_thread_pool = new ThreadPool<http_conn>(m_actor_model, m_conn_pool);
}

/**
 * @brief 初始化数据库连接池
 *
 */
void WebServer::sqlPool() {
  m_conn_pool = connection_pool::GetInstance();
  m_conn_pool->init("localhost", m_user, m_password, m_db_name, 3306,
                    m_connection_pool_size, m_close_log);

  m_users->initmysql_result(m_conn_pool);
}

/**
 * @brief 初始化日志系统
 *
 */
void WebServer::log() {
  if (m_close_log == 0) {
    if (m_log_mode == 1) {
      Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000, 800);
    } else {
      Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000, 0);
    }
  }
}

/**
 * @brief 初始化触发模式
 *
 */
void WebServer::triggerMode() {
  if (m_trigger_mode == 0) {
    m_listen_trigger_mode = 0;
    m_connect_trigger_mode = 0;
  }

  if (m_trigger_mode == 1) {
    m_listen_trigger_mode = 0;
    m_connect_trigger_mode = 1;
  }

  if (m_trigger_mode == 2) {
    m_listen_trigger_mode = 1;
    m_connect_trigger_mode = 0;
  }

  if (m_trigger_mode == 3) {
    m_listen_trigger_mode = 1;
    m_connect_trigger_mode = 1;
  }
}

/**
 * @brief 循环前的预处理
 *
 */
void WebServer::eventListen() {
  // 1. 创建监听套接字
  m_listenfd = socket(AF_INET, SOCK_STREAM, 0);
  assert(m_listenfd != -1);
  // std::cout << "m_listenfd" << m_listenfd << std::endl;

  // 2. 是否设置SO_LINGER选项
  struct linger lig = {0, 1};
  if (m_opt_linger) {
    lig.l_onoff = 1;
  }
  assert(setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &lig, sizeof(lig)) !=
         -1);

  // 3. 给m_listenfd对应的端口号设置SO_REUSADDR选项
  int opt = 1;
  assert(setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) !=
         -1);

  // 4. 将listenfd绑定到指定ip地址以及端口号
  struct sockaddr_in address;
  bzero(&address, sizeof(address));
  address.sin_family = AF_INET;
  // address.sin_addr.s_addr = INADDR_ANY;
  const char* ip = "10.60.29.27";
  inet_pton(AF_INET, ip, &address.sin_addr);
  address.sin_port = htons(m_port);
  assert(bind(m_listenfd, (const sockaddr*)&address, sizeof(address)) != -1);

  // 5. listen, 在accept间隙, 最多接受11个ESTABLISHED连接
  assert(listen(m_listenfd, 10) != -1);

  // 6. 创建信号处理管道
  assert(socketpair(PF_UNIX, SOCK_STREAM, 0, m_sig_pipefd) != -1);
  Utils::setnoblocking(m_sig_pipefd[1]);

  // 7. 将监听套接字注册到内核事件表
  m_epollfd = epoll_create(5);
  assert(m_epollfd != -1);
  Utils::addfd(m_epollfd, m_listenfd, false, (bool)m_listen_trigger_mode);

  // 8. Utils初始化
  Utils::init(m_epollfd, m_sig_pipefd, TIMESLOT);

  // 9. 将信号管道读端注册到内核事件表
  Utils::addfd(m_epollfd, m_sig_pipefd[0], false, 0);

  // 10. 添加信号处理器
  Utils::addsig(SIGPIPE, SIG_IGN);  // SIGPIPE信号会导致进程终止, 必须忽略
  Utils::addsig(SIGALRM, Utils::sig_handler, false);
  Utils::addsig(SIGTERM, Utils::sig_handler, false);

  // 11. http_conn
  http_conn::m_epollfd = m_epollfd;

  // 12. 启动定时器
  // alarm(TIMESLOT);
}

/**
 * @brief 事件循环
 *
 */
void WebServer::eventLoop() {
  bool timeout = false;
  bool stop_server = false;
  bool ret = false;
  struct epoll_event events[MAX_EVENT_NUMBER];

  while (!stop_server) {
    int event_number;
    event_number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
    // errno == EINTR时表示被信号中断
    if (event_number < 0 && errno != EINTR) {
      LOG_ERROR("%s", "epoll failure");
      break;
    }

    for (int i = 0; i < event_number; i++) {
      int sockfd = events[i].data.fd;

      // 新的客户连接
      if (sockfd == m_listenfd) {
        ret = dealNewConnection();
        if (!ret) {
          continue;
        }
      }
      // 错误发生
      else if (events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
        util_timer* timer = m_users_timer[sockfd].timer;
        deleteTimer(timer, sockfd);
      }
      // 接收到信号
      else if ((sockfd == m_sig_pipefd[0]) && (events[i].events & EPOLLIN)) {
        ret = dealSignal(timeout, stop_server);
        if (!ret) {
          LOG_ERROR("%s", "dealclientdata failure");
        }
      }
      // 连接socket可读
      else if (events[i].events & EPOLLIN) {
        dealRead(sockfd);
      }
      // 连接socket可写
      else if (events[i].events & EPOLLOUT) {
        dealWrite(sockfd);
      }
    }

    if (timeout) {
      timeHandler();
      LOG_INFO("%s", "timer tick");

      timeout = false;
    }
  }
}

/**
 * @brief 当用户请求来时, 对其定时器部分进行预处理
 *
 */
void WebServer::timer(int connfd, struct sockaddr_in client_address) {
  m_users[connfd].init(connfd, client_address, m_root, m_connect_trigger_mode,
                       m_close_log, m_user, m_password, m_db_name);
  // 创建一个新的定时器, 将其插入计时器链表当中
  util_timer* timer = new util_timer;
  timer->data = &m_users_timer[connfd];
  timer->cb_func = cb_func;
  time_t cur_timer = time(NULL);
  timer->expire = cur_timer + 3 * TIMESLOT;

  m_users_timer[connfd].client_address = client_address;
  m_users_timer[connfd].sockfd = connfd;
  m_users_timer[connfd].timer = timer;

  m_timer_lst->add_timer(timer);
}

/**
 * @brief 将定时器重置
 *
 */
void WebServer::adjustTimer(util_timer* timer) {
  time_t cur_time = time(NULL);
  timer->expire = cur_time + 3 * TIMESLOT;
  m_timer_lst->adjust_timer(timer);

  LOG_INFO("%s", "adjust timer once");
}

/**
 * @brief  将定时器从定时器链表中删除
 * @details 首先调用定时器回调函数, 然后将其从计时器链表中删除
 * @param timer
 * @param sockfd
 */
void WebServer::deleteTimer(util_timer* timer, int sockfd) {
  timer->cb_func(&m_users_timer[sockfd]);

  if (timer) {
    m_timer_lst->del_timer(timer);
  }

  LOG_INFO("close fd %d", m_users_timer[sockfd].sockfd);
}

bool WebServer::dealNewConnection() {
  std::cout << "dealNewConnection()" << std::endl;
  bool ret = false;
  struct sockaddr_in client_address;

  if (m_listen_trigger_mode == 0) {
    ret = dealNewConnection(client_address);
  } else {
    while (true) {
      ret = dealNewConnection(client_address);
      if (!ret) {
        break;
      }
    }
  }

  return ret;
}

/**
 * @brief 接受一个新的用户连接
 *
 * @param[out] client_address 客户端的地址
 * @return true
 * @return false
 */
bool WebServer::dealNewConnection(struct sockaddr_in& client_address) {
  // std::cout << "deadNewConnection()" << std::endl;
  int connfd;
  socklen_t address_len = sizeof(client_address);
  connfd = accept(m_listenfd, (struct sockaddr*)&client_address, &address_len);

  if (connfd == -1) {
    return false;
  }

  // std::cout << "connfd: " << connfd << std::endl;
  timer(connfd, client_address);

  Utils::addfd(m_epollfd, connfd, true, m_connect_trigger_mode);

  // std::cout << "dealNewConnection() over" << std::endl;
  return true;
}

/**
 * @brief 处理信号事件
 *
 * @param[out] timeout      回传是否为超时信号
 * @param[out] stop_server  回传是否停止服务器
 * @return true
 * @return false
 */
bool WebServer::dealSignal(bool& timeout, bool& stop_server) {
  int signal_cnt;
  int sig;
  char signals[1024];

  signal_cnt = recv(m_sig_pipefd[0], signals, sizeof(signals), 0);
  if (signal_cnt <= 0) {
    return false;
  }

  for (int i = 0; i < signal_cnt; i++) {
    switch (signals[i]) {
      case SIGALRM:
        timeout = true;
        break;
      case SIGTERM:
        stop_server = true;
        break;
      default:
        LOG_INFO("unrecognized signal");
        return false;
    }
  }

  return true;
}

/**
 * @brief 连接套接字可读
 *
 * @param sockfd  连接套接字
 */
void WebServer::dealRead(int sockfd) {
  if (m_actor_model == 1) {
    dealReadReactor(sockfd);
  } else {
    dealReadProactor(sockfd);
  }
}

/**
 * @brief Reactor模式下处理读事件
 * @details 主线程将任务交给线程池中的某个子线程, 然后等待
 * @param sockfd
 */
void WebServer::dealReadReactor(int sockfd) {
  util_timer* timer = m_users_timer[sockfd].timer;

  m_thread_pool->append(&m_users[sockfd], 0);
}

/**
 * @brief Proactor模式下处理读事件
 *
 * @param sockfd
 */
void WebServer::dealReadProactor(int sockfd) {
  std::cout << "dealReadProactor()" << std::endl;
  util_timer* timer = m_users_timer[sockfd].timer;
  if (m_users[sockfd].read()) {
    // LOG_INFO("deal with client(%s)",
    //          inet_ntoa(m_users[sockfd].m_client_address.sin_addr));

    m_thread_pool->append(&m_users[sockfd], 0);
    if (timer) {
      adjustTimer(timer);
    }

    return;
  }

  deleteTimer(timer, sockfd);
}

/**
 * @brief 处理写事件
 *
 * @param sockfd
 */
void WebServer::dealWrite(int sockfd) {
  if (m_actor_model == 1) {
    dealWriteReactor(sockfd);
  } else {
    dealWriteProactor(sockfd);
  }
}

/**
 * @brief Reactor模式下处理写事件
 * @details 主线程将任务交给线程池中的某个子线程, 直接返回
 *          其它逻辑全部由子线程进行处理
 * @param sockfd
 */
void WebServer::dealWriteReactor(int sockfd) {
  util_timer* timer = m_users_timer[sockfd].timer;

  m_thread_pool->append(&m_users[sockfd], 1);
}

/**
 * @brief Proactor模式下处理写事件
 * @details 主线程直接write()将缓冲区的数据发送给客户
 *
 * @param sockfd 连接套接字
 */
void WebServer::dealWriteProactor(int sockfd) {
  std::cout << "dewlWriteProactor()" << std::endl;
  util_timer* timer = m_users_timer[sockfd].timer;

  if (m_users[sockfd].write()) {
    LOG_INFO("send data to the client(%s)",
             inet_ntoa(m_users[sockfd].m_client_address.sin_addr));
    if (timer) {
      adjustTimer(timer);
    }

    return;
  }

  deleteTimer(timer, sockfd);
}

void WebServer::timeHandler() {
  m_timer_lst->tick();
  alarm(TIMESLOT);
}