#ifndef CONFIG_H
#define CONFIG_H

#include "server/server.h"

class Config {
  public:
   Config();
   ~Config(){};

   void parse_arg(int argc, char* argv[]);

   // 端口号
   int port;

   // 日志写入方式
   int log_mode;

   // 触发组合模式
   int trigger_mode;

   // listenfd触发模式
   int listen_trigger_mode;

   // connfd触发模式
   int connect_trigger_mode;

   // 优雅关闭链接
   int opt_linger;

   // 数据库连接池中的连接数量
   int connection_pool_size;

   // 线程池内的线程数量
   int thread_num;

   // 是否关闭日志
   int close_log;

   // 并发模型选择
   int actor_model;
};

#endif