cmake_minimum_required(VERSION 2.8)
 
# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "-std=c++0x -Wall -Werror -g")
# Link runTests with what we want to test and the GTest and pthread library
add_executable(error_handling_test test/tests.cpp)
target_link_libraries(error_handling_test ${GTEST_LIBRARIES} pthread)

enable_testing()
add_test(NAME    error_handling_test 
         COMMAND error_handling_test)


