cmake_minimum_required(VERSION 3.15)
project(beth LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# beth.hpp のクラスと、DLL の呼び出し層(internal/。利用者は直接使わない。docs/adr/0032)。
add_library(beth STATIC beth.cpp internal/api.cpp)
target_include_directories(beth PUBLIC ${PROJECT_SOURCE_DIR})
# DLL を実行時に読み込む(Linux の dlopen)。
target_link_libraries(beth PRIVATE ${CMAKE_DL_LIBS})

# Windows では、Bethany をリンクした exe に manifest(Common-Controls 6.0 の宣言)を埋め込む。
# LCL は OS の標準コントロールを作るため、その外観は exe の manifest で決まり、無いと旧来のクラシックな外観になる。
# 自前の manifest を持つ exe では OFF にする(manifest のリソースが重複するため)。
option(BETH_EMBED_MANIFEST "Embed a Common-Controls 6.0 manifest into executables that link Bethany" ON)
if(WIN32 AND BETH_EMBED_MANIFEST)
    enable_language(RC)
    set(BETH_MANIFEST_PATH "${PROJECT_SOURCE_DIR}/win32/beth.manifest")
    configure_file(win32/beth_manifest.rc.in "${PROJECT_BINARY_DIR}/beth_manifest.rc" @ONLY)
    # INTERFACE のソースは、Bethany をリンクした側(exe)でコンパイル・リンクされる。
    target_sources(beth INTERFACE "${PROJECT_BINARY_DIR}/beth_manifest.rc")
endif()

add_subdirectory(test)
