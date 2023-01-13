#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <mysql/mysql.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include "CGImysql/sql_connection_pool.h"
#include "log/log.h"
#include "utils/utils.h"

class http_conn {
  public:
   static const int FILE_NAME_LEN = 200;    // 读取文件的名称大小
   static const int READ_BUF_SIZE = 2048;   // m_read_buf大小
   static const int WRITE_BUF_SIZE = 1024;  // m_write_buf大小

   enum METHOD {
      GET = 0,
      HEAD,
      POST,
      PUT,
      DELETE,
      CONNECT,
      OPTIONS,
      TRACE,
      PATCH,
   };

   enum CHECK_STATE {
      CHECK_STATE_REQUEST_LINE = 0,
      CHECK_STATE_REQUEST_HEADER,
      CHECK_STATE_REQUEST_BODY,
   };

   enum LINE_STATUS {
      LINE_OK = 0,
      LINE_BAD,
      LINE_OPEN,  // 当前行还没有完整写入m_read_buf
   };

   enum HTTP_CODE {
      NO_REQUEST,         // 请求不完整
      GET_REQUEST,        // 获取到了完整请求
      BAD_REQUEST,        // 客户请求有语法错误
      NO_RESOURCE,        // 服务器上找不到资源
      FORBIDDEN_REQUEST,  // 客户对资源没有足够的访问权限
      FILE_REQUEST,       // 文件请求
      INTERNAL_ERROR,     // 服务器内部错误
      CLOSED_CONNECTION   // 请求关闭连接
   };

  public:
   http_conn();
   ~http_conn();

  public:
   void init(int conn_fd, const sockaddr_in& client_addr, char* root,
             bool enable_et, int close_log, const std::string& user,
             const std::string& passwd, const std::string& dbname);

   void close_conn();
   void process();
   bool read();
   bool write();

   void initmysql_result(connection_pool* conn_pool);

   // public:  // for test
  private:
   void init();
   // read()的辅助函数
   bool read_once();

   HTTP_CODE process_read();
   bool process_write(HTTP_CODE read_ret);

   // process_read() 的辅助函数
   std::pair<bool, char*> parse_request_line_method(char* start);
   std::pair<bool, char*> parse_request_line_url(char* start);
   std::pair<bool, char*> parse_request_line_version(char* start);
   HTTP_CODE parse_request_line(char* request_line);
   HTTP_CODE parse_request_header(char* request_header);
   HTTP_CODE parse_request_body(char* request_body);
   LINE_STATUS parse_line();
   HTTP_CODE do_request();

   // process_write()的辅助函数
   void unmap();
   bool add_response(const char* format, ...);
   bool add_response_line(int status, const char* title);
   bool add_response_header(int content_length);
   bool add_response_body(const char* body);
   bool add_content_length(int content_length);
   bool add_linger();
   bool add_blank_line();

  public:
   MYSQL* m_mysql;
   static int m_epollfd;     // 静态内核事件表
   static int m_user_count;  // 系统中所有活跃的用户数目
   int m_state;              // 对数据库的读写类型, 读为0, 写为1

  private:
   char m_read_buf[READ_BUF_SIZE];    // http请求读缓冲区
   char m_write_buf[WRITE_BUF_SIZE];  // http响应写缓冲区
   int m_read_index;  // 下一次写入到m_read_buf的开始位置
   int m_checked_index;  // 下一次对m_read_buf使用parse_line()预处理的开始位置
   int m_write_index;  // 下一次写入到m_write_buf的位置

   int m_connfd;                  // 连接套接字
   sockaddr_in m_client_address;  // 客户端地址

   static std::map<std::string, std::string> m_users;
   std::string m_user;    // sql用户名
   std::string m_passwd;  // sql密码
   std::string m_dbname;  // sql数据库名字

   bool m_enable_et;  // 是否启用ET模式
   int m_close_log;   // 是否关闭日志
   char* m_doc_root;  // 该服务文档树的根部路径
   int m_cgi;

   char* m_url;                      // 请求行中的请求资源
   char m_real_file[FILE_NAME_LEN];  // 请求文件对应的服务器真实路径
   char* m_file_address;             // 文件映射在内存中的地址
   struct stat m_file_stat;          // 请求文件的状态

   METHOD m_method;            // 请求方法
   char* m_version;            // 请求报文使用的http版本
   CHECK_STATE m_check_state;  // 当前主状态机的状态
   int m_content_length;       // 请求体的内容
   bool m_linger;              // 是否继续保持连接
   char* m_host;               // 服务器的主机和端口号
   char* m_body;               // 消息体的起始位置

   struct iovec m_iv[2];  // 主线程将其中的内容写入socket
   int m_iv_count;        // iovec结构体的个数

   int m_bytes_to_send;  // 主线程在该socket上已经发送了多少字节
   int m_bytes_have_send;  // 主线程还需要在该socket上发送多少字节
};

#endif
