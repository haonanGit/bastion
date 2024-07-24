cmake_minimum_required(VERSION 3.10...3.22)

project(logging_benchmark)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set compile flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Find the required libraries
find_package(spdlog REQUIRED)
find_package(benchmark REQUIRED)

# Add executable
add_executable(bm_spdlog bm_spdlog.cpp)

# Link libraries
target_link_libraries(bm_spdlog spdlog::spdlog benchmark::benchmark)
