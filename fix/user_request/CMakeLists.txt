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
add_executable(user_request user_request.cpp ../../common.cpp)

# 链接库
target_link_libraries(user_request PRIVATE quickfix OpenSSL::SSL OpenSSL::Crypto)
