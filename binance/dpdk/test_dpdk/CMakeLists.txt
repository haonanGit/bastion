cmake_minimum_required(VERSION 3.10)

project(DPDKExample)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mssse3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mssse3 -g")

# 查找DPDK库
find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

# 设置DPDK库路径
include_directories(${DPDK_INCLUDE_DIRS})
link_directories(${DPDK_LIBRARY_DIRS})

# 打印调试信息
message(STATUS "DPDK include directories: ${DPDK_INCLUDE_DIRS}")
message(STATUS "DPDK library directories: ${DPDK_LIBRARY_DIRS}")
message(STATUS "DPDK libraries: ${DPDK_LIBRARIES}")

# 服务器源文件
set(SERVER_SOURCES
test_dpdk.cpp
)

# 编译服务器
add_executable(test_dpdk ${SERVER_SOURCES})
target_link_libraries(test_dpdk ${DPDK_LIBRARIES})
