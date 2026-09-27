cmake_minimum_required(VERSION 3.15)
project(no_vcl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# no_vcl.hpp のクラスと、DLL の呼び出し層(internal/。利用者は直接使わない。docs/adr/0032)。
add_library(no_vcl STATIC no_vcl.cpp internal/api.cpp)
target_include_directories(no_vcl PUBLIC ${PROJECT_SOURCE_DIR})
# DLL を実行時に読み込む(Linux の dlopen)。
target_link_libraries(no_vcl PRIVATE ${CMAKE_DL_LIBS})

add_subdirectory(test)
