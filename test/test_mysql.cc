#include <string>
#include "CGImysql/sql_connection_pool.h"
#include "gtest/gtest.h"

TEST(Testmysql, test_connection_pool_create) {
   Log* log = Log::get_instance();
   log->init("web_srv.log", 0);

   connection_pool* pool = connection_pool::GetInstance();
   std::string url = "localhost";    // 不能够使用127.0.0.1
   std::string user = "root";
   std::string password = "123456";
   std::string database_name = "server_db";
   int port = 3306;
   int max_conn = 20;
   int close_log = 0;

   pool->init(url, user, password, database_name, port, max_conn, close_log);

   EXPECT_EQ(pool->GetFreeConnCnt(), 20);

}

TEST(Testmysql, test_connection_get) {
   Log* log = Log::get_instance();
   log->init("web_srv.log", 0);

   connection_pool* pool = connection_pool::GetInstance();
   std::string url = "localhost";  // 不能够使用127.0.0.1
   std::string user = "root";
   std::string password = "123456";
   std::string database_name = "server_db";
   int port = 3306;
   int max_conn = 20;
   int close_log = 0;

   pool->init(url, user, password, database_name, port, max_conn, close_log);

   for (int i = 0; i < 20; i ++) {
      MYSQL* conn = NULL;
      connectionRAII(&conn, pool);
      EXPECT_FALSE(conn == NULL);
   }
}
