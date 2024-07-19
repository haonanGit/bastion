cmake_minimum_required(VERSION 3.10...3.22)

project (benchmark_ringbuffer)

# 指定 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

include_directories(../)

add_executable(bm_ringbuffer bm_ringbuffer.cpp)

link_directories(/usr/local/lib64)
target_link_libraries(bm_ringbuffer benchmark pthread)



