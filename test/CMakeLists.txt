set(GTEST_LIB gtest gtest_main)

# 1 log test
add_executable(test_log ${CMAKE_CURRENT_SOURCE_DIR}/test_log.cc
                        ${PROJECT_SOURCE_DIR}/log/log.cc)
target_include_directories(
  test_log
  PUBLIC ${PROJECT_SOURCE_DIR}
  PUBLIC ${PROJECT_SOURCE_DIR}/third_party/googletest/googletest/include)
target_link_libraries(test_log ${GTEST_LIB})
target_compile_options(test_log PRIVATE -g)

# 2 mysql test
add_executable(
  test_mysql
  ${CMAKE_CURRENT_SOURCE_DIR}/test_mysql.cc
  ${PROJECT_SOURCE_DIR}/CGImysql/sql_connection_pool.cc
  ${PROJECT_SOURCE_DIR}/log/log.cc)
target_include_directories(
  test_mysql
  PUBLIC ${PROJECT_SOURCE_DIR}
  PUBLIC ${PROJECT_SOURCE_DIR}/third_party/googletest/googletest/include)
target_link_libraries(
  test_mysql
  PUBLIC ${GTEST_LIB}
  PUBLIC mysqlclient)
target_compile_options(test_mysql PRIVATE -g)

# 3 timer list test
add_executable(test_lst_timer ${CMAKE_CURRENT_SOURCE_DIR}/test_lst_timer.cc
                              ${PROJECT_SOURCE_DIR}/log/log.cc)
target_include_directories(
  test_lst_timer
  PUBLIC ${PROJECT_SOURCE_DIR}
  PUBLIC ${PROJECT_SOURCE_DIR}/third_party/googletest/googletest/include)
target_link_libraries(test_lst_timer PUBLIC ${GTEST_LIB})
target_compile_options(test_lst_timer PRIVATE -g)

# 4 http test
add_executable(
  test_http
  ${CMAKE_CURRENT_SOURCE_DIR}/test_http.cc
  ${PROJECT_SOURCE_DIR}/http/http_conn.cc
  ${PROJECT_SOURCE_DIR}/log/log.cc
  ${PROJECT_SOURCE_DIR}/CGImysql/sql_connection_pool.cc
  ${PROJECT_SOURCE_DIR}/utils/utils.cc)
target_include_directories(
  test_http
  PUBLIC ${PROJECT_SOURCE_DIR}
  PUBLIC ${PROJECT_SOURCE_DIR}/third_party/googletest/googletest/include)
target_link_libraries(
  test_http
  PUBLIC ${GTEST_LIB}
  PUBLIC mysqlclient)
target_compile_options(test_http PRIVATE -g)
