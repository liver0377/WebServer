cmake_minimum_required(VERSION 3.16)

project(my_tiny_web_server VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)

add_executable(
  websrv
  main.cc ${PROJECT_SOURCE_DIR}/log/log.cc
  ${PROJECT_SOURCE_DIR}/CGImysql/sql_connection_pool.cc
  ${PROJECT_SOURCE_DIR}/http/http_conn.cc
  ${PROJECT_SOURCE_DIR}/utils/utils.cc)

add_subdirectory(locker)
add_subdirectory(log)
add_subdirectory(test)
add_subdirectory(CGImysql)
add_subdirectory(timer)
add_subdirectory(utils)
add_subdirectory(http)
add_subdirectory(third_party/googletest)

# websrv构建时的include路径为项目根目录
target_include_directories(websrv PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(
  websrv
  PUBLIC pthread
  PUBLIC mysqlclient)

target_compile_options(websrv PRIVATE -g)

# 设置输出路径为项目根目录
set_target_properties(
  websrv
  PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
             LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
             RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

# test
enable_testing()
add_test(NAME test_log COMMAND test_log)
add_test(NAME test_mysql COMMAND test_mysql)
add_test(NAME test_lst_timer COMMAND test_lst_timer)
add_test(NAME test_http COMMAND test_http)
