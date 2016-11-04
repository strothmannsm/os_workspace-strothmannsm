cmake_minimum_required (VERSION 2.8)
project(allocation)

set(CMAKE_CXX_FLAGS "-std=c++0x -g -Wall -Werror")

# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(${PROJECT_NAME}_tester test/tests.cpp src/${PROJECT_NAME}.c)
target_link_libraries(${PROJECT_NAME}_tester ${GTEST_LIBRARIES} pthread)
