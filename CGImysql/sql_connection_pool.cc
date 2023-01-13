#include "CGImysql/sql_connection_pool.h"
#include <pthread.h>

connection_pool::connection_pool() : m_curconn(0), m_freeconn(0) {}

connection_pool::~connection_pool() { DestroyPool(); }

MYSQL* connection_pool::GetConnection() {
   lock.lock();

   if (m_conn_list.empty()) {
      lock.unlock();
      return NULL;
   }

   lock.unlock();
   reserve.wait();

   MYSQL* conn = NULL;
   conn = m_conn_list.front();
   m_conn_list.pop_front();

   lock.lock();

   m_freeconn--;
   m_curconn++;

   lock.unlock();

   return conn;
}

bool connection_pool::ReleaseConnection(MYSQL* conn) {
   if (conn == NULL) {
      return false;
   }

   lock.lock();

   m_conn_list.push_back(conn);
   m_freeconn++;
   m_curconn--;

   lock.unlock();

   reserve.post();
   return true;
}

int connection_pool::GetFreeConnCnt() { return m_freeconn; }

void connection_pool::DestroyPool() {
   lock.lock();

   for (auto conn : m_conn_list) {
      mysql_close(conn);
   }

   m_curconn = 0;
   m_freeconn = 0;
   lock.unlock();
}

connection_pool* connection_pool::GetInstance() {
   static connection_pool pool;
   return &pool;
}

void connection_pool::init(const std::string& url, const std::string& user,
                           const std::string& password,
                           const std::string& database_name, int port,
                           int maxconn, int close_log) {

   m_url = url;
   m_user = user;
   m_password = password;
   m_database_name = database_name;
   m_port = port;
   m_close_log = close_log;

   for (int i = 0; i < maxconn; i++) {
      MYSQL* conn = NULL;

      conn = mysql_init(NULL);
      if (conn == NULL) {
         LOG_ERROR("MYSQL CONNECTION INIT ERROR");
         exit(-1);
      }

      conn = mysql_real_connect(conn, m_url.c_str(), m_user.c_str(),
                                m_password.c_str(), m_database_name.c_str(),
                                m_port, NULL, 0);
      if (conn == NULL) {
         LOG_ERROR("MYSQL CONNECTION ESTABLISH ERROR");
         exit(-1);
      }

      m_conn_list.push_back(conn);
      m_freeconn++;
   }

   m_maxconn = m_freeconn;
   reserve = sem(m_freeconn);
}

connectionRAII::connectionRAII(MYSQL** conn, connection_pool* pool) {
   *conn = pool->GetConnection();

   m_connRAII = *conn;
   m_pollRAII = pool;
}

connectionRAII::~connectionRAII() { m_pollRAII->ReleaseConnection(m_connRAII); }
