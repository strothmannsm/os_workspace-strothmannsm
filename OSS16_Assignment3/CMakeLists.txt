cmake_minimum_required(VERSION 2.8)

set(CMAKE_C_FLAGS "-std=c99 -Wall -Wextra -Wshadow -Werror -g -D_XOPEN_SOURCE=500")

add_executable(producer-consumer src/producer_consumer.c)