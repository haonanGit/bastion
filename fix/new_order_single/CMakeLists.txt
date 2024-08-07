cmake_minimum_required(VERSION 3.10)

# 项目信息
project(Fix VERSION 1.0 LANGUAGES CXX)

# 指定 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 包含目录
include_directories(../../quickfix/include/ ./ ../ ../../ ../../../ )

# 查找 OpenSSL 库
find_package(OpenSSL REQUIRED)

# 添加可执行文件
add_executable(new_order_single new_order_single.cpp ../../common.cpp)

# 链接库
target_link_libraries(new_order_single PRIVATE quickfix OpenSSL::SSL OpenSSL::Crypto)
