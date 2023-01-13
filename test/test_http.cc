#include <string>
#include "gtest/gtest.h"
#include "http/http_conn.h"

// TEST(TestHttp, test_parse_request_line) {
//    http_conn hc;
//    char str1[] = "GET /image.png HTTP/1.1";
// 
//    http_conn::HTTP_CODE ret;
//    ret = hc.parse_request_line(str1);
//    EXPECT_EQ(ret, http_conn::NO_REQUEST);
// 
//    char str2[] = "XXX /image.png HTTP/1.1";
//    ret = hc.parse_request_line(str2);
//    EXPECT_EQ(ret, http_conn::BAD_REQUEST);
// 
//    char str3[] = "POST http://baidu.com/test/index.html HTTP/1.1";
//    ret = hc.parse_request_line(str3);
//    EXPECT_EQ(ret, http_conn::NO_REQUEST);
// 
//    char str4[] = "GET /index.html HTTP/1.2";
//    ret = hc.parse_request_line(str4);
//    EXPECT_EQ(ret, http_conn::BAD_REQUEST);
// }
// 
// TEST(TestHttp, test_prase_request_header) {
//    http_conn hc;
//    http_conn::HTTP_CODE ret;
// 
//    char str1[] = "Connection: keep-alive";
//    ret = hc.parse_request_header(str1);
//    EXPECT_EQ(ret, http_conn::NO_REQUEST);
// 
//    char str2[] = "Host: www.baidu.com:80";
//    ret = hc.parse_request_header(str2);
//    EXPECT_EQ(ret, http_conn::NO_REQUEST);
// 
//    char str3[] = "";
//    ret = hc.parse_request_header(str3);
//    EXPECT_EQ(ret, http_conn::GET_REQUEST);
// 
//    char str4[] = "Content-length: 20";
//    ret = hc.parse_request_header(str4);
//    EXPECT_EQ(ret, http_conn::NO_REQUEST);
// 
//    char str5[] = "";
//    ret = hc.parse_request_header(str5);
//    EXPECT_EQ(ret, http_conn::NO_REQUEST);
// }
// 
// TEST(TestHttp, test_parse_request_body) {
//    http_conn hc;
//    char str[] = "aaaaabbbbb\0";
//    hc.m_content_length = 10;
// 
//    http_conn::HTTP_CODE ret;
//    // 1. body内容还没有写入完全
//    hc.m_checked_index = 0;
//    hc.m_read_index = 9;
//    ret = hc.parse_request_body(str);
//    EXPECT_EQ(ret, http_conn::NO_REQUEST);
// 
//    // 2. body内容完整
//    hc.m_checked_index = 0;
//    hc.m_read_index = 10;
//    ret = hc.parse_request_body(str);
//    EXPECT_EQ(hc.m_body, str);
//    EXPECT_EQ(http_conn::GET_REQUEST, ret);
// }
// 
// TEST(TestHttp, test_parse_line) {
//    http_conn hc;
//    http_conn::LINE_STATUS ret;
// 
//    char str1[] = "abcdefg\r\n\0";
//    memset(hc.m_read_buf, 0, sizeof(hc.m_read_buf));
//    hc.m_checked_index = 0;
//    hc.m_read_index = strlen(str1);
//    strcpy(hc.m_read_buf, str1);
//    ret = hc.parse_line();
//    EXPECT_EQ(ret, http_conn::LINE_OK);
// 
//    char str2[] = "abcdefg\r\nhijk\0";
//    memset(hc.m_read_buf, 0, sizeof(hc.m_read_buf));
//    hc.m_checked_index = 0;
//    hc.m_read_index = strlen(str2);
//    strcpy(hc.m_read_buf, str2);
//    ret = hc.parse_line();
//    EXPECT_EQ(ret, http_conn::LINE_OK);
// 
//    char str3[] = "abcdefg\r\0";
//    memset(hc.m_read_buf, 0, sizeof(hc.m_read_buf));
//    hc.m_checked_index = 0;
//    hc.m_read_index = strlen(str3);
//    strcpy(hc.m_read_buf, str3);
//    ret = hc.parse_line();
//    EXPECT_EQ(ret, http_conn::LINE_OPEN);
// 
//    char str4[] = "abcdefg\rhi\0";
//    memset(hc.m_read_buf, 0, sizeof(hc.m_read_buf));
//    hc.m_checked_index = 0;
//    hc.m_read_index = strlen(str4);
//    strcpy(hc.m_read_buf, str4);
//    ret = hc.parse_line();
//    EXPECT_EQ(ret, http_conn::LINE_BAD);
// }
// 
// TEST(TestHttp, test_do_request) {
//    // TODO
// }
// 
// TEST(TestHttp, test_add_response) {
//    const char* ok_200_title = "OK";
//    const char* error_400_title = "Bad Request";
//    const char* error_400_form =
//        "Your request has bad syntax or is inherently impossible to staisfy.\n";
//    const char* error_403_title = "Forbidden";
//    const char* error_403_form =
//        "You do not have permission to get file form this server.\n";
//    const char* error_404_title = "Not Found";
//    const char* error_404_form =
//        "The requested file was not found on this server.\n";
//    const char* error_500_title = "Internal Error";
//    const char* error_500_form =
//        "There was an unusual problem serving the request file.";
// 
//    http_conn hc;
//    Log* log = Log::get_instance();
//    log->init("web_srv.log", 0, 8192, 500000, 0);
// 
//    hc.add_response_line(200, ok_200_title);
//    EXPECT_EQ(strcmp(hc.m_write_buf, "HTTP/1.1 200 OK\r\n"), 0);
// 
//    hc.add_response_header(10);
//    EXPECT_EQ(
//        strcmp(
//            hc.m_write_buf,
//            "HTTP/1.1 200 OK\r\nContent-Length:10\r\nConnection:close\r\n\r\n"),
//        0);
// 
//    hc.add_response_body("aaaaabbbbb");
//    EXPECT_EQ(
//        strcmp(hc.m_write_buf,
//               "HTTP/1.1 200 "
//               "OK\r\nContent-Length:10\r\nConnection:close\r\n\r\naaaaabbbbb"),
//        0);
// }
