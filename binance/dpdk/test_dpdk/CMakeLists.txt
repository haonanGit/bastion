cmake_minimum_required(VERSION 3.10)

project(dpdk)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 找到DPDK
find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

# 包含DPDK头文件
include_directories(${DPDK_INCLUDE_DIRS})

# 设置DPDK库路径
link_directories(${DPDK_LIBRARY_DIRS})

# 添加可执行文件
add_executable(test_dpdk test_dpdk.cpp)

# 链接DPDK库
target_link_libraries(test_dpdk ${DPDK_LIBRARIES})
