# beth_deploy(<target>)
#
# Bethany をリンクした実行ファイルのビルドの後に、その隣へ Bethany の共有ライブラリ(beth.dll / libbeth.so)を写す。
# beth.dll は実行時に読み込まれる(LoadLibrary・dlopen)ため、リンクだけでは exe の隣に置かれない。
#
# 写すファイルは、グローバルなプロパティ BETH_DLL が示すもの(ソースからビルドしたときは CMakeLists.txt の
# キャッシュ変数 BETH_DLL、インストールした先では bethConfig.cmake が設定する)。
function(beth_deploy target)
    get_property(dll GLOBAL PROPERTY BETH_DLL)
    if(NOT dll)
        message(FATAL_ERROR "beth_deploy: the path of the Bethany shared library (BETH_DLL) is not set")
    endif()
    # POST_BUILD では exe をリンクし直したときにしか写らず、beth.dll だけを作り直したときに古いものが残るため、
    # ビルドのたびに(内容が変わったときだけ)写すターゲットにする。
    add_custom_target(${target}_beth_deploy ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "$<TARGET_FILE_DIR:${target}>"
        VERBATIM)
    add_dependencies(${target}_beth_deploy ${target})
endfunction()
