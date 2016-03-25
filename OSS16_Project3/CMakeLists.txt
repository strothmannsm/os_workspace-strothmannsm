cmake_minimum_required(VERSION 2.8)
project(S16FS)

set(CMAKE_COLOR_MAKEFILE ON)

if(APPLE)
	# Make policy CMP0042 warning go away
	# even though I still don't quite understand it
	set(CMAKE_MACOSX_RPATH ON)
endif(APPLE)

find_library(back_store_lib back_store)
find_library(bitmap_lib bitmap)
find_library(dyn_array_lib dyn_array)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS} include)

# add -DENABLE_MOVE to the CXX flags to activate move tests
set(SHARED_FLAGS " -Wall -Wextra -Wshadow -g -D_POSIX_C_SOURCE=200809L ")
set(CMAKE_CXX_FLAGS " -std=c++11 ${SHARED_FLAGS} ")
set(CMAKE_C_FLAGS " -std=c99 ${SHARED_FLAGS} ")

add_executable(fs_test test/tests.cpp)
target_link_libraries(fs_test SoneSixFS ${back_store_lib} ${dyn_array_lib} ${bitmap_lib} ${GTEST_LIBRARIES} pthread)

enable_testing()
add_test(NAME    fs_test 
         COMMAND fs_test)
