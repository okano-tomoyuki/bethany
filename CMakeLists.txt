cmake_minimum_required(VERSION 3.15)
project(no_vcl LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(no_vcl_c STATIC no_vcl_c.cpp)

add_library(no_vcl STATIC no_vcl.cpp)
target_link_libraries(no_vcl PRIVATE no_vcl_c)

add_subdirectory(test)
