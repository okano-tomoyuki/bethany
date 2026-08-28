add_executable(test_c main.c)
target_include_directories(test_c PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(test_c PRIVATE no_vcl_c)
# main.cはCソースだが no_vcl_c(C++実装)にリンクするため、最終リンクはC++リンカで行う。
set_target_properties(test_c PROPERTIES LINKER_LANGUAGE CXX)

add_executable(test_cpp main.cpp)
target_include_directories(test_cpp PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(test_cpp PRIVATE no_vcl)
