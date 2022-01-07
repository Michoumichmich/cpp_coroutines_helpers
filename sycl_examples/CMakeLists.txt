cmake_minimum_required(VERSION 3.4)
project(sycl_coroutines)

set(CMAKE_CXX_STANDARD 20)

include(cmake/FindSYCL.cmake)

include_directories(../include)
add_executable(sycl_coroutines main.cpp)
add_sycl_to_target(TARGET sycl_coroutines SOURCES main.cpp)