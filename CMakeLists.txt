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

# Windows では、no_vcl をリンクした exe に manifest(Common-Controls 6.0 の宣言)を埋め込む。
# LCL は OS の標準コントロールを作るため、その外観は exe の manifest で決まり、無いと旧来のクラシックな外観になる。
# 自前の manifest を持つ exe では OFF にする(manifest のリソースが重複するため)。
option(NO_VCL_EMBED_MANIFEST "Embed a Common-Controls 6.0 manifest into executables that link no_vcl" ON)
if(WIN32 AND NO_VCL_EMBED_MANIFEST)
    enable_language(RC)
    set(NO_VCL_MANIFEST_PATH "${PROJECT_SOURCE_DIR}/win32/no_vcl.manifest")
    configure_file(win32/no_vcl_manifest.rc.in "${PROJECT_BINARY_DIR}/no_vcl_manifest.rc" @ONLY)
    # INTERFACE のソースは、no_vcl をリンクした側(exe)でコンパイル・リンクされる。
    target_sources(no_vcl INTERFACE "${PROJECT_BINARY_DIR}/no_vcl_manifest.rc")
endif()

add_subdirectory(test)
