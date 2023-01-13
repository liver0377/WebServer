#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <string>
#include "log/log.h"
#include "thread/thread_pool.h"
#include "timer/sort_timer_lst.h"

const int MAX_USER_COUNT = 65536;    // 最大活跃用户数目
const int MAX_EVENT_NUMBER = 10000;  // 最大事件数
const int TIMESLOT = 5;              // 最小超时单位

class WebServer {
  public:
   WebServer();
   ~WebServer();

   void init(int port, const std::string &user, const std::string &password,
             const std::string &db_name, int log_mode, int opt_linger,
             int trigger_mode, int connection_pool_size, int thread_num, int close_log,
             int actor_model);
   void threadPool();
   void sqlPool();
   void logWrite();
   void triggerMode();
   void eventListen();
   void eventLoop();
   void timer(int connfd, struct sockaddr_in client_address);
   void adjustTimer(util_timer *timer);
   void dealTimer(util_timer *timer, int sockfd);
   bool dealClientData();
   bool dealWithSignal();
   void dealWithRead(int sockfd);
   void dealWithWrite(int sockfd);

  public:
   int m_port;         // 服务器运行的端口
   char *m_root;       // 资源文件的根目录路径
   int m_log_mode;     // 日志模式
   int m_close_log;    // 是否关闭日志
   int m_actor_model;  // 事件处理模式

   int m_pipefd[2];
   int m_epollfd;       // 内核事件表描述符
   http_conn *m_users;  // 所有的客户连接

   // 数据库
   connection_pool *m_conn_pool;  // 数据库连接池
   std::string m_user;            // 用户名
   std::string m_password;        // 用户密码
   std::string m_db_name;         // 数据库名称
   int m_connection_pool_size;

   // 线程池
   ThreadPool<http_conn> *m_thread_pool;
   int m_thread_num;

   epoll_event events[MAX_EVENT_NUMBER];
   int m_listenfd;    // 监听套接字
   int m_opt_linger;  // 是否启用SO_LINGER选项
   int m_trigger_mode;
   int m_listen_trigger_mode;   // 监听套接字触发模式
   int m_connect_trigger_mode;  // 连接套接字触发模式

   // 定时器相关
   client_data *m_users_timer;  // 每个连接都有一个定时器
   Utils m_utils;
};
#endif