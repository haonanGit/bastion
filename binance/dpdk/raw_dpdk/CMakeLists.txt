cmake_minimum_required(VERSION 3.10)

project(DPDKExample)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mssse3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mssse3")


# 设置DPDK路径
set(DPDK_PATH "/usr/local/share/dpdk")

# 查找DPDK库
find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

# 设置DPDK库路径
include_directories(${DPDK_INCLUDE_DIRS})
link_directories(${DPDK_LIBRARY_DIRS})

# 客户端源文件
set(CLIENT_SOURCES
    client.cpp
)

# 服务器源文件
set(SERVER_SOURCES
    server.cpp
)

# 编译客户端
add_executable(client ${CLIENT_SOURCES})
target_link_libraries(client ${DPDK_LIBRARIES})

# 编译服务器
add_executable(server ${SERVER_SOURCES})
target_link_libraries(server ${DPDK_LIBRARIES})
