cmake_minimum_required(VERSION 3.10...3.22)

project(websocket_app)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set compile flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# Include directories
include_directories(../ ../../ ../../json/single_include/)

# Find the Boost libraries
find_package(Boost REQUIRED COMPONENTS system thread)

if(Boost_FOUND)
    # Include Boost header directories
    include_directories(${Boost_INCLUDE_DIRS})
    # Disable Boost auto-linking feature
    add_definitions(-DBOOST_ALL_NO_LIB)
endif()

# Add executable
add_executable(test_biance aggregate_trade_streams.cpp ws_client.cpp ../common.cpp)

# Link Boost libraries and pthread
target_link_libraries(test_biance ${Boost_LIBRARIES} pthread ssl crypto)
