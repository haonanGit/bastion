cmake_minimum_required(VERSION 3.10...3.22)

project(websocket_app)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set compile flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2")

# Include directories
include_directories(../ ../../ ../../../json/single_include/ ../../logger)

# Find the Boost libraries
find_package(Boost REQUIRED COMPONENTS system thread)

if(Boost_FOUND)
    # Include Boost header directories
    include_directories(${Boost_INCLUDE_DIRS})
    # Disable Boost auto-linking feature
    add_definitions(-DBOOST_ALL_NO_LIB)
endif()

# Find spdlog library
find_package(spdlog REQUIRED)

# Add executable
add_executable(aggtrade_trade_sport aggtrade_trade_sport.cpp ../ws_client.cpp ../../common.cpp ../../logger/logger.cpp)

# Link Boost libraries and pthread
target_link_libraries(aggtrade_trade_sport ${Boost_LIBRARIES} pthread ssl crypto spdlog::spdlog)
