cmake_minimum_required(VERSION 3.15)
# バージョンは C++ の配布物(package-windows.sh の zip)と CHANGELOG.md に合わせる(docs/adr/0038)。
project(beth VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# このリポジトリを直接ビルドしているときだけテストを作る(配布の zip は test/ を含まない)。
# FetchContent・add_subdirectory で取り込まれたときは、テストもインストールの規則も作らない。
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(BETH_IS_TOP_LEVEL ON)
else()
    set(BETH_IS_TOP_LEVEL OFF)
endif()
if(BETH_IS_TOP_LEVEL AND EXISTS "${PROJECT_SOURCE_DIR}/test/CMakeLists.txt")
    set(BETH_HAS_TESTS ON)
else()
    set(BETH_HAS_TESTS OFF)
endif()
option(BETH_BUILD_TESTS "Build the Bethany tests" ${BETH_HAS_TESTS})
option(BETH_INSTALL "Generate the install rules (headers, library, beth.dll and the CMake package)" ${BETH_IS_TOP_LEVEL})

# beth.hpp のクラスと、DLL の呼び出し層(internal/。利用者は直接使わない。docs/adr/0032)。
# 利用者のコンパイラでビルドする(配布するのはソースと beth.dll。docs/adr/0038)。
# ヘッダは include/bethany/ に置き、利用者は #include <bethany/beth.hpp> と書く(システムインクルード。docs/adr/0040)。
add_library(beth STATIC src/beth.cpp src/internal/api.cpp)
add_library(beth::beth ALIAS beth)
target_include_directories(beth PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
# DLL を実行時に読み込む(Linux の dlopen)。
target_link_libraries(beth PRIVATE ${CMAKE_DL_LIBS})

# MinGW の g++ でリンクした exe は libgcc_s_seh-1.dll・libstdc++-6.dll・libwinpthread-1.dll に依存する。
# これらを exe に静的にリンクし、配る物を exe と beth.dll だけにする。
option(BETH_STATIC_RUNTIME "Link the MinGW runtime (libgcc, libstdc++, winpthread) statically into executables that link Bethany" ON)
if(MINGW AND BETH_STATIC_RUNTIME)
    target_link_options(beth INTERFACE -static)
endif()

# FPC でビルドした共有ライブラリ(build-windows.sh・build-linux.sh が作る。配布の zip には同梱する)。
if(WIN32)
    set(BETH_DLL_NAME beth.dll)
else()
    set(BETH_DLL_NAME libbeth.so)
endif()
# BETH_DLL は利用者が別の DLL を指定するためのもの。空(既定)なら、このソースのフォルダの DLL を使う。
# 既定の値をキャッシュに保存すると、FetchContent の取り込み先(FETCHCONTENT_SOURCE_DIR_BETH 等)を変えたときに古い DLL を写し続けるため、
# 既定の値は configure のたびに決める。
set(BETH_DLL "" CACHE FILEPATH "The Bethany shared library built by FPC (empty: the one in the source folder of Bethany)")
# 0.2.0 までは既定の値(そのときのソースのフォルダの DLL)をキャッシュに保存していた。取り込み先を変えた後も古い DLL を写さないよう、
# 別の Bethany のソースのフォルダ(beth.pas がある)の DLL を指していれば、以前の既定の値とみなして捨てる。
if(BETH_DLL)
    get_filename_component(BETH_DLL_DIR "${BETH_DLL}" DIRECTORY)
    get_filename_component(BETH_DLL_DIR "${BETH_DLL_DIR}" REALPATH)
    get_filename_component(BETH_SOURCE_DIR_REAL "${PROJECT_SOURCE_DIR}" REALPATH)
    if(NOT BETH_DLL_DIR STREQUAL BETH_SOURCE_DIR_REAL AND EXISTS "${BETH_DLL_DIR}/beth.pas")
        message(STATUS "Bethany: BETH_DLL pointed to another Bethany source (${BETH_DLL}); using ${PROJECT_SOURCE_DIR}")
        set(BETH_DLL "" CACHE FILEPATH "The Bethany shared library built by FPC (empty: the one in the source folder of Bethany)" FORCE)
    endif()
endif()
if(BETH_DLL)
    set(BETH_DLL_PATH "${BETH_DLL}")
else()
    set(BETH_DLL_PATH "${PROJECT_SOURCE_DIR}/${BETH_DLL_NAME}")
endif()
set_property(GLOBAL PROPERTY BETH_DLL "${BETH_DLL_PATH}")
include(cmake/BethDeploy.cmake)

# Windows では、Bethany をリンクした exe に manifest(Common-Controls 6.0 の宣言)を埋め込む。
# LCL は OS の標準コントロールを作るため、その外観は exe の manifest で決まり、無いと旧来のクラシックな外観になる。
# 自前の manifest を持つ exe では OFF にする(manifest のリソースが重複するため)。
option(BETH_EMBED_MANIFEST "Embed a Common-Controls 6.0 manifest into executables that link Bethany" ON)
if(WIN32 AND BETH_EMBED_MANIFEST)
    enable_language(RC)
    set(BETH_MANIFEST_PATH "${PROJECT_SOURCE_DIR}/win32/beth.manifest")
    configure_file(win32/beth_manifest.rc.in "${PROJECT_BINARY_DIR}/beth_manifest.rc" @ONLY)
    # INTERFACE のソースは、Bethany をリンクした側(exe)でコンパイル・リンクされる。
    # インストールした先では、bethConfig.cmake が利用者のビルドフォルダに .rc を作り直す(絶対パスを埋めるため)。
    target_sources(beth INTERFACE $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/beth_manifest.rc>)
endif()

if(BETH_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(TARGETS beth EXPORT bethTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(DIRECTORY include/bethany DESTINATION include)
    if(WIN32)
        install(FILES "${BETH_DLL_PATH}" DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
    else()
        install(FILES "${BETH_DLL_PATH}" DESTINATION ${CMAKE_INSTALL_LIBDIR} OPTIONAL)
    endif()
    install(FILES win32/beth.manifest win32/beth_manifest.rc.in cmake/BethDeploy.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/beth)
    install(FILES LICENSE THIRD-PARTY-NOTICES.md DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/beth)
    install(DIRECTORY licenses/ DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/beth/licenses)

    install(EXPORT bethTargets NAMESPACE beth:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/beth)
    configure_package_config_file(cmake/bethConfig.cmake.in "${PROJECT_BINARY_DIR}/bethConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/beth)
    # 0.x の間は、マイナーバージョンが変わると互換が無いものとする。
    write_basic_package_version_file("${PROJECT_BINARY_DIR}/bethConfigVersion.cmake"
        COMPATIBILITY SameMinorVersion)
    install(FILES "${PROJECT_BINARY_DIR}/bethConfig.cmake" "${PROJECT_BINARY_DIR}/bethConfigVersion.cmake"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/beth)
endif()

if(BETH_BUILD_TESTS)
    add_subdirectory(test)
endif()
