# DLL の関数を内部層(beth::internal)から直接呼ぶテスト(docs/adr/0032)。
add_executable(test_internal internal/funcs.cpp)
target_link_libraries(test_internal PRIVATE beth)
beth_deploy(test_internal)

add_executable(test_cpp main.cpp)
target_link_libraries(test_cpp PRIVATE beth)
beth_deploy(test_cpp)
