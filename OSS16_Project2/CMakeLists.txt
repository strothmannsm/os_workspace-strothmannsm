cmake_minimum_required(VERSION 2.8)


find_library(back_store_lib back_store)

# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Link runTests with what we want to test and the GTest and pthread library
set(CMAKE_CXX_FLAGS "-std=c++0x -Wall -Wextra -Wshadow -Werror -g")
# Link runTests with what we want to test and the GTest and pthread library
add_executable(page_swap_test test/tests.cpp)
target_link_libraries(page_swap_test ${back_store_lib} ${GTEST_LIBRARIES} pthread)

enable_testing()
add_test(NAME    page_swap_test 
         COMMAND page_swap_test)





