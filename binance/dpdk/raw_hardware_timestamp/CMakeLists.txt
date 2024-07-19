cmake_minimum_required(VERSION 3.10)

# 项目信息
project(TimestampTest)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 包含必要的头文件路径
include_directories(/usr/include)

# 设置源文件
set(SERVER_SOURCES server.cpp)
set(CLIENT_SOURCES client.cpp)

# 生成可执行文件
add_executable(server ${SERVER_SOURCES})
add_executable(client ${CLIENT_SOURCES})

# 链接必要的库
target_link_libraries(server pthread)
