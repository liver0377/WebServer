#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL

#include <mysql/mysql.h>
#include <list>
#include <string>
#include "locker/locker.h"
#include "log/log.h"

/**
 * @brief 数据库连接池
 *
 */
class connection_pool {
  public:
   /**
    * @brief 从连接池中获取到某一个连接
    *
    * @return MYSQL*
    */
   MYSQL *GetConnection();

   /**
    * @brief 将指定的数据库连接放回数据库连接池
    *
    * @param conn    指定数据库连接
    * @return true   释放成功
    * @return false  释放失败
    */
   bool ReleaseConnection(MYSQL *conn);

   /**
    * @brief 返回数据库连接池中空闲连接的数量
    *
    * @return int
    */
   int GetFreeConnCnt();

   /**
    * @brief 销毁所有连接
    *
    */
   void DestroyPool();

   /**
    * @brief 获取到数据库连接池实例
    *
    * @return connection_pool*
    */
   static connection_pool *GetInstance();

   /**
    * @brief 对数据库连接池进行初始化
    *
    * @param url        数据库主机ip地址
    * @param user       登录用户名
    * @param password   登录用户的密码
    * @param database_name  登录的数据库的名字
    * @param port       登录的端口号
    * @param max_conn   数据库连接池中的最大连接数目
    * @param close_log  是否关闭日志
    */
   void init(const std::string &url, const std::string &user,
             const std::string &password, const std::string &database_name,
             int port, int max_conn, int close_log);

  private:
   connection_pool();
   ~connection_pool();

   int m_maxconn;   // 最大连接数
   int m_curconn;   // 当前已经使用的连接数
   int m_freeconn;  // 当前空闲的连接数
   locker lock;
   sem reserve;  // 不同进程获取连接时使用信号量进行同步
   std::list<MYSQL *> m_conn_list;  // 所有空闲连接

  public:
   std::string m_url;            // 数据库主机地址
   int m_port;                   // 连接数据库时使用的端口号
   std::string m_user;           // 登录数据库的用户名
   std::string m_password;       // 登录数据库的密码
   std::string m_database_name;  // 数据库名称
   int m_close_log;              // 日志开关
};

/**
 * @brief 单个连接的RAII封装类
 *
 */
class connectionRAII {
  public:
   /**
    * @brief 构造connectionRAII类
    *
    * @param[out] conn 获取到的数据库连接, 用于返回
    * @param connPool  数据库连接池
    */
   connectionRAII(MYSQL **conn, connection_pool *connPool);
   ~connectionRAII();

  private:
   MYSQL *m_connRAII;
   connection_pool *m_pollRAII;
};
#endif