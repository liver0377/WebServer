#include "server/server.h"

WebServer::WebServer()
    : m_users(new http_conn[MAX_USER_COUNT]),
      m_users_timer(new client_data[MAX_USER_COUNT]) {

   char server_root[200];
   if (getcwd(server_root, 200) == NULL) {
      throw std::exception();
   }

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

void WebServer::threadPool() {
   m_thread_pool = new ThreadPool<http_conn>(m_actor_model, m_conn_pool);
}

void WebServer::sqlPool() {
   m_conn_pool = connection_pool::GetInstance();
   m_conn_pool->init("localhost", m_user, m_password, m_db_name, 3306, m_connection_pool_size, m_close_log);
   
   // ??
   m_users->initmysql_result(m_conn_pool);
}

void WebServer::eventListen() {
   // 1. 创建监听套接字
   m_listenfd = socket(AF_INET, SOCK_STREAM, 0);
   assert(m_listenfd != -1);

   // 2. 是否设置SO_LINGER选项
   struct linger lig = {0, 1};
   if (m_opt_linger) {
      lig.l_linger = 1;
   }
   assert(setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &lig, sizeof (lig)) != -1);
   
   // 3. 将listenfd绑定到指定ip地址以及端口号
   struct sockaddr_in address; 
   bzero(&address, sizeof (address));
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port = htonl(m_port);
   bind(m_listenfd, (const sockaddr* )&address, sizeof (address));

   // 4. 给m_listenfd对应的端口号设置SO_REUSADDR选项
   int opt = 1;
   assert(setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt)) != -1);

   // TODO
}