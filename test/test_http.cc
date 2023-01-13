#include <string>
#include "gtest/gtest.h"
#include "http/http_conn.h"
// 
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
