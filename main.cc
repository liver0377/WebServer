#include "config/config.h"

int main(int argc, char *argv[]) {

   std::string user = "root";
   std::string password = "root";
   std::string db_name = "server_db";

   // 命令行参数解析
   Config config;
   config.parse_arg(argc, argv);

   // 服务器运行
   WebServer server;

   // 1. 初始化参数
   server.init(config.port, user, password, db_name, config.log_mode,
               config.opt_linger, config.trigger_mode, config.connection_pool_size,
               config.thread_num, config.close_log, config.actor_model);

   // 2. 初始化日志系统
   server.logWrite();

   // 3. 初始化数据库连接池
   server.sqlPool();

   // 4. 初始化线程池
   server.threadPool();

   // 4. 设置监听描述符和连接描述符的配置模式
   server.triggerMode();

   // 4. 监听事件
   server.eventListen();

   // 5. 事件循环
   server.eventLoop();

   return 0;
}