cmake_minimum_required(VERSION 3.5)

project (sw_server)

#给头文件指定路径用的
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(./basedefine)
add_subdirectory(./http_module)
add_subdirectory(./mysql)
add_subdirectory(./thread_pool)
add_subdirectory(./timer)
add_subdirectory(./log)

add_compile_options(-std=c++14 -g -Wall)


aux_source_directory(. SW_SERVER_SRC)
add_executable(sw_server ${SW_SERVER_SRC})
target_link_libraries(sw_server pthread mysqlclient)