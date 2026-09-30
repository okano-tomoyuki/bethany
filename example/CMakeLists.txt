# Bethany Notepad(C++)のビルド。フォームのコードは Bethany Designer が cpp/ に生成したもの。
#
#   cmake -S . -B build -G Ninja
#   cmake --build build
#   build/Notepad.exe
#
# Bethany は Releases の zip を FetchContent で取り込む。このリポジトリの中のライブラリ(開発中のもの)を使うときは、
# 取り込む代わりにそのフォルダを指定する(CMake の標準の仕組み。先に build-windows.sh で beth.dll をビルドしておく):
#
#   cmake -S . -B build -G Ninja -DFETCHCONTENT_SOURCE_DIR_BETH=..
cmake_minimum_required(VERSION 3.15)
project(Notepad CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
# CMake 3.25 以降は、Bethany のヘッダをシステムインクルード(-isystem)にして、その警告を出さない
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.25)
    set(BETH_FETCH_SYSTEM SYSTEM)
endif()
FetchContent_Declare(beth
    URL https://github.com/okano-tomoyuki/bethany/releases/download/v0.3.0/bethany-0.3.0-win64.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    ${BETH_FETCH_SYSTEM})
FetchContent_MakeAvailable(beth)

# WIN32: コンソールの窓を出さない GUI のアプリケーションにする
add_executable(Notepad WIN32
    cpp/Notepad.cpp
    cpp/MainForm.cpp
    cpp/dialogs/AboutForm.cpp)
# 生成したコードは、ヘッダを cpp/ からのパスで include する(プロジェクトの codegen.cpp.headerDir)
target_include_directories(Notepad PRIVATE cpp)
target_link_libraries(Notepad PRIVATE beth::beth)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Notepad PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
# beth.dll を Notepad.exe の隣に写す
beth_deploy(Notepad)
