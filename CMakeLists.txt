cmake_minimum_required(VERSION 3.21)
project(coroutines_tests)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

add_executable(coro main.cpp)
add_executable(tests tests.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -stdlib=libc++ -fcoroutines-ts -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined,address")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

