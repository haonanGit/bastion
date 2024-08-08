cmake_minimum_required(VERSION 3.10)

project(biance)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set compile flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -Werror")  # Enable debug symbols and warnings

# Include directories
include_directories(../ ../../ ../../../ ../../../json/single_include/)

# Find the Boost libraries
find_package(Boost REQUIRED COMPONENTS system thread)

if(Boost_FOUND)
    # Include Boost header directories
    include_directories(${Boost_INCLUDE_DIRS})
    # Disable Boost auto-linking feature
    add_definitions(-DBOOST_ALL_NO_LIB)
endif()

# Add executable for cancellation
add_executable(cancellation cancellation.cpp ../../common.cpp)

# Add executable for merge_deribit
add_executable(merge_deribit merge_deribit.cpp ../../common.cpp)

# Link pthread library
find_package(Threads REQUIRED)
target_link_libraries(cancellation PRIVATE Threads::Threads ${Boost_LIBRARIES} pthread ssl crypto)
target_link_libraries(merge_deribit PRIVATE Threads::Threads ${Boost_LIBRARIES} pthread ssl crypto)


# Set core dump settings
add_custom_command(TARGET cancellation POST_BUILD
    COMMAND ulimit -c 2097152   && echo "/tmp/core/core.%e.%p" | sudo tee /proc/sys/kernel/core_pattern
    COMMENT "Enable core dumps"
)

# Set core dump settings for merge_deribit
add_custom_command(TARGET merge_deribit POST_BUILD
    COMMAND ulimit -c 2097152 && echo "/tmp/core/core.%e.%p" | sudo tee /proc/sys/kernel/core_pattern
    COMMENT "Enable core dumps"
)
