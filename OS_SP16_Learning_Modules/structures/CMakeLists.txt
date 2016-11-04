cmake_minimum_required (VERSION 2.8)
project(structures)

set(CMAKE_CXX_FLAGS "-std=c++0x -g -Wall -Werror")

# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
 
# Link runTests with what we want to test and the GTest and pthread library
add_executable(${PROJECT_NAME}_tester test/test.cpp src/${PROJECT_NAME}.c)
target_link_libraries(${PROJECT_NAME}_tester ${GTEST_LIBRARIES} pthread)

enable_testing()
add_test(NAME    ${PROJECT_NAME}_test 
         COMMAND ${PROJECT_NAME}_test)
