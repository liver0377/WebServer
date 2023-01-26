#include "http/http_conn.h"

int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;
std::map<std::string, std::string>* http_conn::m_users_map =
    new std::map<std::string, std::string>;
// sort_timer_lst http_conn::m_timer_lst = sort_timer_lst();

const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form =
    "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form =
    "You do not have permission to get file form this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form =
    "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form =
    "There was an unusual problem serving the request file.";

http_conn::http_conn() {}

http_conn::~http_conn() {}

/**
 * @brief 初始化
 *
 * @param conn_fd   连接套接字
 * @param client_addr  客户端套接字地址
 * @param root
 * @param enable_et  是否启用ET模式
 * @param close_log  是否关闭日志
 * @param user       数据库用户名
 * @param passwd     用户密码
 * @param dbname     数据库名
 */
void http_conn::init(int connfd, const sockaddr_in& client_addr, char* doc_root,
                     bool enable_et, int close_log, const std::string& user,
                     const std::string& passwd, const std::string& dbname) {
  // std::cout << "http_conn::init()" << std::endl;
  // std::cout << "connfd: " << connfd << std::endl;
  m_connfd = connfd;
  m_client_address = client_addr;
  m_enable_et = enable_et;
  m_close_log = close_log;
  m_doc_root = doc_root;

  memset(m_user, '\0', 100);
  memset(m_passwd, '\0', 100);
  memset(m_dbname, '\0', 100);
  strcpy(m_user, user.c_str());
  strcpy(m_passwd, passwd.c_str());
  strcpy(m_dbname, dbname.c_str());
  //   m_user = user;
  //   m_passwd = passwd;
  //   m_dbname = dbname;

  //   memset(m_doc_root, '\0', 1024);
  //   strcpy(m_doc_root, doc_root);

  init();
}

void http_conn::close_conn() {
  // std::cout << "close_conn()" << std::endl;
  Utils::removefd(m_epollfd, m_connfd);
  m_connfd = -1;
  m_user_count--;
}

void http_conn::process() {
  // std::cout << "process()" << std::endl;
  HTTP_CODE read_ret = process_read();
  if (read_ret == NO_REQUEST) {
    // 请求报文还没有读完
    Utils::modifyfd(m_epollfd, m_connfd, EPOLLIN, m_enable_et);
    return;
  }

  // 当请求报文有错误时, 需要回送错误信息
  // if (read_ret == BAD_REQUEST) {
  //    return ;
  // }

  bool write_ret = process_write(read_ret);
  if (!write_ret) {
    close_conn();
  }

  // std::cout << "m_epollfd: " << m_epollfd << "m_connfd: " << m_connfd
  //           << std::endl;
  Utils::modifyfd(m_epollfd, m_connfd, EPOLLOUT, m_enable_et);
  // std::cout << "process() over" << std::endl;
}

void http_conn::init() {
  m_mysql = NULL;
  m_bytes_to_send = 0;
  m_bytes_have_send = 0;
  m_check_state = CHECK_STATE_REQUEST_LINE;
  m_linger = false;
  m_method = GET;
  m_url = 0;
  m_version = 0;
  m_content_length = 0;
  m_host = 0;
  m_body = 0;
  m_checked_index = 0;
  m_read_index = 0;
  m_write_index = 0;
  m_state = 0;
  // timer_flag = 0;
  // improv = 0;

  memset(m_read_buf, '\0', READ_BUF_SIZE);
  memset(m_write_buf, '\0', WRITE_BUF_SIZE);
  memset(m_real_file, '\0', FILE_NAME_LEN);
}

/**
 * @brief 从连接套接字读取一次数据到http读缓冲区
 *
 * @return true 读到有效字节
 * @return false 对端已关闭或者没有数据可读
 */
bool http_conn::read_once() {
  int bytes_read = 0;
  bytes_read = recv(m_connfd, m_read_buf + m_read_index,
                    READ_BUF_SIZE - m_read_index, 0);

  if (bytes_read == -1) {
    if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
      return false;
    }
  }

  if (bytes_read == 0) {
    return false;
  }

  m_read_index += bytes_read;

  return true;
}

/**
 * @brief 被主线程所调用, 从连接套接字读取数据
 *        如果没有启用ET模式, 那么只会读取一次
 *        如果启用ET模式, 那么会反复读取
 * @return true:  读取到数据
 *         false: 缓冲区满 | 对端关闭了连接 | 没有数据可读 | 其它非法错误
 */
bool http_conn::read() {
  // std::cout << "read()" << std::endl;
  if (m_read_index >= READ_BUF_SIZE) {
    return false;
  }

  bool ans = true;
  if (m_enable_et) {
    while (ans) {
      ans = read_once();
    }
  } else {
    ans = read_once();
  }

  return ans;
}

/**
 * @brief 被主线程所调用,
 * 将http写缓冲区中的数据以及文件内容通过连接socket发送给客户
 *
 * @return true  继续保持连接
 * @return false 断开连接
 */
bool http_conn::write() {
  // std::cout << "begin to write()" << std::endl;
  int temp = 0;

  if (m_bytes_to_send == 0) {
    Utils::modifyfd(m_epollfd, m_connfd, EPOLLIN, m_enable_et);
    init();
    return true;
  }

  while (true) {
    temp = writev(m_connfd, m_iv, m_iv_count);

    if (temp < 0) {
      // 写操作阻塞, 说明TCP写缓冲区满了, 那么只能够等待下一轮EPOLLOUT事件
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        Utils::modifyfd(m_epollfd, m_connfd, EPOLLOUT, m_enable_et);
        return true;
      }

      // 写操作出错, 解除文件的映射
      unmap();
      return false;
    }

    m_bytes_have_send += temp;
    m_bytes_to_send -= temp;

    if (m_bytes_have_send >= m_iv[0].iov_len) {
      m_iv[0].iov_base = 0;
      m_iv[0].iov_len = 0;
      m_iv[1].iov_base = m_file_address + m_bytes_have_send - m_write_index;
      m_iv[1].iov_len = m_bytes_to_send;
    } else {
      m_iv[0].iov_base = m_write_buf + m_bytes_have_send;
      m_iv[0].iov_len = m_iv[0].iov_len - m_bytes_have_send;
    }

    if (m_bytes_to_send <= 0) {
      unmap();
      Utils::modifyfd(m_epollfd, m_connfd, EPOLLIN, m_enable_et);
      if (m_linger) {
        init();
        return true;
      }
      return false;
    }
  }

  return true;
}

/**
 * @brief 首先和数据库连接连接, 获取到所有的用户和密码
 *
 * @param conn_pool
 */
void http_conn::initmysql_result(connection_pool* conn_pool) {
  MYSQL* mysql = NULL;
  connectionRAII mysqlcon(&mysql, conn_pool);

  // 在user表中检索username，passwd数据，浏览器端输入
  if (mysql_query(mysql, "SELECT username,passwd FROM user")) {
    LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
  }

  // 从表中检索完整的结果集
  MYSQL_RES* result = mysql_store_result(mysql);

  // 返回结果集中的列数
  int num_fields = mysql_num_fields(result);

  // 返回所有字段结构的数组
  MYSQL_FIELD* fields = mysql_fetch_fields(result);

  // 从结果集中获取下一行，将对应的用户名和密码，存入map中
  while (MYSQL_ROW row = mysql_fetch_row(result)) {
    std::string temp1(row[0]);
    std::string temp2(row[1]);
    m_users_map->insert({temp1, temp2});
  }
}

std::pair<bool, char*> http_conn::parse_request_line_method(char* start) {
  char* p;
  p = strpbrk(start, " \t");
  if (p == NULL) {
    return {false, NULL};
  }

  *p++ = '\0';
  if (strcasecmp(start, "GET") == 0) {
    m_method = GET;
  } else if (strcasecmp(start, "POST") == 0) {
    m_method = POST;
  } else {
    return {false, NULL};
  }

  return {true, p};
}

std::pair<bool, char*> http_conn::parse_request_line_url(char* start) {
  start += strspn(start, " \t");

  char* p;
  p = strpbrk(start, " \t");
  if (p == NULL) {
    return {false, NULL};
  }

  *p++ = '\0';
  m_url = start;

  // m_url以http://开头
  if (strncasecmp(m_url, "http://", 7) == 0) {
    m_url += 7;
    m_url = strchr(m_url, '/');
  }

  // m_url以https://开头     存疑
  if (strncasecmp(m_url, "https://", 8) == 0) {
    m_url += 8;
    m_url = strchr(m_url, '/');
  }

  if (m_url == NULL || m_url[0] != '/') {
    return {false, NULL};
  }

  if (strlen(m_url) == 1) {
    strcat(m_url, "judge.html");
  }

  return {true, p};
}

std::pair<bool, char*> http_conn::parse_request_line_version(char* start) {
  start += strspn(start, " \t");

  char* p = NULL;
  m_version = start;

  if (strcasecmp(m_version, "HTTP/1.1") != 0) {
    return {false, NULL};
  }

  return {true, p};
}

/**
 * @brief 解析请求行
 *
 * @param request_line  请求行, 以'\0'结尾
 * @return HTTP_CODE
 */
http_conn::HTTP_CODE http_conn::parse_request_line(char* request_line) {
  // std::cout << "parse_request_line()" << std::endl;
  std::pair<bool, char*> ret;

  ret = parse_request_line_method(request_line);
  if (ret.first == false) {
    return BAD_REQUEST;
  }

  ret = parse_request_line_url(ret.second);
  if (ret.first == false) {
    return BAD_REQUEST;
  }

  ret = parse_request_line_version(ret.second);
  if (ret.first == false) {
    return BAD_REQUEST;
  }

  // std::cout << "m_method" << m_method << std::endl;
  // std::cout << "m_url" << m_url << std::endl;
  // std::cout << "m_version" << m_version << std::endl;

  m_check_state = CHECK_STATE_REQUEST_HEADER;
  return NO_REQUEST;
}

/**
 * @brief  解析请求头
 *
 * @param request_header  请求头, 以'\0'结尾
 * @return HTTP_CODE
 */
http_conn::HTTP_CODE http_conn::parse_request_header(char* request_header) {
  // std::cout << "parse_request_header()" << std::endl;
  // 该行为空行
  if (request_header[0] == '\0') {
    if (m_content_length > 0) {
      m_check_state = CHECK_STATE_REQUEST_BODY;
      return NO_REQUEST;
    }
    // std::cout << "m_content_length = 0" << std::endl;
    return GET_REQUEST;
  }

  if (strncasecmp(request_header, "Connection:", 11) == 0) {
    request_header += 11;
    request_header += strspn(request_header, " \t");
    if (strncasecmp(request_header, "keep-alive", 10) == 0) {
      m_linger = true;
    }
  } else if (strncasecmp(request_header, "Content-length:", 15) == 0) {
    request_header += 15;
    request_header += strspn(request_header, " \t");
    m_content_length = atol(request_header);
  } else if (strncasecmp(request_header, "Host:", 5) == 0) {
    request_header += 5;
    request_header += strspn(request_header, " \t");
    m_host = request_header;
  } else {
    LOG_INFO("oops! unkonwn header: %s", request_header);
  }

  return NO_REQUEST;
}

/**
 * @brief 解析请求头
 *
 * @param request_body  请求头, 以'\0'结尾
 * @return HTTP_CODE
 */
http_conn::HTTP_CODE http_conn::parse_request_body(char* request_body) {
  // std::cout << "parse_request_body()" << std::endl;
  // 消息体还没有读完
  if (m_read_index < m_content_length + m_checked_index) {
    return NO_REQUEST;
  }

  request_body[m_content_length] = '\0';

  // std::cout << "m_body is set" << std::endl;
  m_body = request_body;

  return GET_REQUEST;
}

/**
 * @brief 从m_read_buf中解析每段以\r\n结尾的字符串
 * @details parse_line()会解析请求头中的每行字符串,
 * 同时将行尾的\r\n替换为\0\0
 *
 * @return LINE_STATUS
 */
http_conn::LINE_STATUS http_conn::parse_line() {
  // std::cout << "parse_line()" << std::endl;
  char ch;
  for (; m_checked_index < m_read_index; m_checked_index++) {
    ch = m_read_buf[m_checked_index];

    if (ch == '\r') {
      if (m_checked_index + 1 == m_read_index) {
        return LINE_OPEN;
      }

      if (m_read_buf[m_checked_index + 1] == '\n') {
        m_read_buf[m_checked_index] = '\0';
        m_read_buf[m_checked_index + 1] = '\0';
        m_checked_index += 2;
        return LINE_OK;
      }

      return LINE_BAD;
    }
    // 当上一个字节\r是已读数据的最后一个字节时, 那么控制流就会进入该分支
    else if (ch == '\n') {
      if (m_checked_index > 1 && m_read_buf[m_checked_index - 1] == '\r') {
        m_read_buf[m_checked_index] = '\0';
        m_read_buf[m_checked_index - 1] = '\0';
        m_checked_index++;
        return LINE_OK;
      }

      return LINE_BAD;
    }
  }

  return LINE_OPEN;
}

/**
 * @brief 子进程使用该函数解析http请求读缓冲区的内容
 *
 * @return HTTP_CODE
 */
http_conn::HTTP_CODE http_conn::process_read() {
  // std::cout << "process_read()" << std::endl;
  char* text = NULL;
  int start_index = 0;
  LINE_STATUS line_status = LINE_OK;
  HTTP_CODE ret = NO_REQUEST;

  // m_check_state == CHECK_STATE_REQUEST_BODY && line_stateus == LINE_OK:
  // 表明刚刚检查完空行, 准备检查请求体 对请求体的解析不应该进行parse_line()
  while (
      (m_check_state == CHECK_STATE_REQUEST_BODY && line_status == LINE_OK) ||
      (line_status = parse_line()) == LINE_OK) {
    // std::cout << "text - m_read_buf" << text - m_read_buf << std::endl;
    // std::cout << "start_index" << start_index << std::endl;
    text = m_read_buf + start_index;
    start_index = m_checked_index;
    LOG_INFO("%s", text);

    switch (m_check_state) {
      case CHECK_STATE_REQUEST_LINE:
        ret = parse_request_line(text);
        if (ret == BAD_REQUEST) {
          return BAD_REQUEST;
        }
        break;

      case CHECK_STATE_REQUEST_HEADER:
        ret = parse_request_header(text);
        if (ret == BAD_REQUEST) {
          return BAD_REQUEST;
        } else if (ret == GET_REQUEST) {
          return do_request();
        }
        break;

      case CHECK_STATE_REQUEST_BODY:
        ret = parse_request_body(text);
        if (ret == GET_REQUEST) {
          return do_request();
        }
        line_status = LINE_OPEN;  // body的内容还没有读完, 准备退出循环
        break;

      default:
        break;
    }
  }

  if (line_status == LINE_OPEN) {
    return NO_REQUEST;
  }

  return BAD_REQUEST;
}

/**
 * @brief 当子进程读取到一个完整的http请求报文之后, 会执行该函数
 * @details 该函数会检查文件的权限, 类型, 最后将其映射到内存中的某个地址
 * @return HTTP_CODE
 */
// 待修改
http_conn::HTTP_CODE http_conn::do_request() {
  // std::cout << "do_request()" << std::endl;
  strcpy(m_real_file, m_doc_root);

  int type = -1;  // 0: login, 1: register, 2: file request 
  if (strcmp(m_url, "/login") == 0) {
    type = 0;
  } else if (strcmp(m_url, "/register") == 0) {
    type = 1;
  } else {
    type = 2;
  }

  char url[100] = {0};
  strcpy(url, m_url);

  // std::cout << "type: " << type << std::endl;
  // std::cout << "m_body: " << m_body << std::endl;

  if (type == 0 || type == 1) {
    char name[100];
    char passwd[100];

    int i;
    for (i = 5; m_body[i] != '&'; ++i) {
      name[i - 5] = m_body[i];
    }
    name[i - 5] = '\0';

    int j = 0;
    for (i = i + 10; m_body[i] != '\0'; ++i, ++j) {
      passwd[j] = m_body[i];
    }
    passwd[j] = '\0';

    // std::cout << "user: " << name << "passwd: " << passwd << std::endl;
    // 登录
    if (type == 0) {
      // std::cout << "m_users_map size: " << m_users_map->size() << std::endl;
      if (m_users_map->find(name) != m_users_map->end() &&
          (*m_users_map)[name] == passwd) {
        strcpy(url, "/welcome.html");
      } else {
        // std::cout << "log error" << std::endl;
        strcpy(url, "/logError.html");
      }
    }

    // 注册
    if (type == 1) {
      char* sql_insert = (char*)malloc(sizeof(char) * 200);
      strcpy(sql_insert, "INSERT INTO user(username, passwd) VALUES(");
      strcat(sql_insert, "'");
      strcat(sql_insert, name);
      strcat(sql_insert, "', '");
      strcat(sql_insert, passwd);
      strcat(sql_insert, "')");

      if (m_users_map->find(name) == m_users_map->end()) {
        int res = mysql_query(m_mysql, sql_insert);
        m_lock.lock();
        m_users_map->insert(std::pair<std::string, std::string>(name, passwd));
        m_lock.unlock();

        if (!res) {
          strcpy(url, "/log.html");
        } else {
          strcpy(url, "/registerError.html");
        }
      } else {
        strcpy(url, "/registerError.html");
      }
    }
  }

  int len = strlen(m_doc_root);

  strncpy(m_real_file + len, url, FILE_NAME_LEN - len - 1);
  // std::cout << "m_real_file" << m_real_file << std::endl;

  if (stat(m_real_file, &m_file_stat) < 0) {
    return NO_RESOURCE;
  }

  if (!(m_file_stat.st_mode & S_IROTH)) {
    return FORBIDDEN_REQUEST;
  }

  if (S_ISDIR(m_file_stat.st_mode)) {
    return BAD_REQUEST;
  }

  int fd = open(m_real_file, O_RDONLY);
  m_file_address =
      (char*)mmap(NULL, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  assert(m_file_address != MAP_FAILED);
  close(fd);

  // std::cout << "url : " << url << " request file: " << m_real_file << std::endl;
  // std::cout << "do_request() over" << std::endl;
  return FILE_REQUEST;
}

void http_conn::unmap() {
  if (m_file_address) {
    int ret;
    ret = munmap(m_file_address, m_file_stat.st_size);
    assert(ret != -1);
  }
}
/**
 * @brief 工具函数, 用于将参数按照format指定的格式写入到写缓冲区中
 *
 * @param format
 * @param ...
 * @return true
 * @return false
 */
bool http_conn::add_response(const char* format, ...) {
  if (m_write_index >= WRITE_BUF_SIZE) {
    return false;
  }

  va_list arg_list;
  va_start(arg_list, format);

  int len = vsnprintf(m_write_buf + m_write_index,
                      WRITE_BUF_SIZE - m_write_index - 1, format, arg_list);
  if (len >= WRITE_BUF_SIZE - m_write_index - 1) {
    va_end(arg_list);
    return false;
  }

  m_write_index += len;

  va_end(arg_list);

  LOG_INFO("request: %s", m_write_buf);
  return true;
}
bool http_conn::add_response_line(int status, const char* title) {
  return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool http_conn::add_response_header(int content_length) {
  return add_content_length(content_length) && add_linger() && add_blank_line();
}

bool http_conn::add_response_body(const char* body) {
  return add_response("%s", body);
}

bool http_conn::add_content_length(int content_length) {
  return add_response("Content-Length:%d\r\n", content_length);
}

bool http_conn::add_linger() {
  return add_response("Connection:%s\r\n", m_linger ? "keep-alive" : "close");
}

bool http_conn::add_blank_line() { return add_response("%s", "\r\n"); }

/**
 * @brief 子线程根据请求解析的情况, 做出不同的写操作
 *        1. 错误: 向http写缓冲区写入错误信息
 *        2. 文件请求: 将m_iv[0],
 * m_iv[1]的起始地址分别指定为写缓冲区首地址和内存中文件映射的首地址
 *
 * @param read_ret
 * @return true
 * @return false
 */
bool http_conn::process_write(HTTP_CODE read_ret) {
  // std::cout << "process_write()" << std::endl;
  switch (read_ret) {
    case INTERNAL_ERROR:
      add_response_line(500, error_500_title);
      add_response_header(strlen(error_500_form));
      if (!add_response_body(error_500_form)) {
        return false;
      }
      break;

    case BAD_REQUEST:
      add_response_line(404, error_404_title);
      add_response_header(strlen(error_404_form));
      if (!add_response_body(error_404_form)) {
        return false;
      }
      break;

    case FORBIDDEN_REQUEST:
      add_response_line(403, error_403_title);
      add_response_header(strlen(error_403_form));
      if (!add_response_body(error_403_form)) {
        return false;
      }
      break;

    case FILE_REQUEST:
      // std::cout << "good request" << std::endl;
      add_response_line(200, ok_200_title);
      if (m_file_stat.st_size > 0) {
        add_response_header(m_file_stat.st_size);
        m_iv[0].iov_base = m_write_buf;
        m_iv[0].iov_len = m_write_index;
        m_iv[1].iov_base = m_file_address;
        m_iv[1].iov_len = m_file_stat.st_size;
        m_iv_count = 2;

        m_bytes_to_send = m_write_index + m_file_stat.st_size;
        return true;
      } else {
        const char* ok_string = "<html><body></body></html>";
        add_response_header(strlen(ok_string));
        if (!add_response_body(ok_string)) {
          return false;
        }
      }

    default:
      return false;
  }

  // 非文件请求
  m_iv[0].iov_base = m_write_buf;
  m_iv[0].iov_len = m_write_index;
  m_iv_count = 1;
  m_bytes_to_send = m_write_index;
  // std::cout << "非文件请求" << std::endl;
  return true;
}
