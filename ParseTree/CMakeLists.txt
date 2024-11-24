add_library(PARSE_TREE SHARED ParseTree.c)
target_include_directories(PARSE_TREE PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(PARSE_TREE PRIVATE sanitizer_flags Utility)

# soft link
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/ParseTree.h ${CMAKE_CURRENT_BINARY_DIR}/ParseTree.h)

if (PARSE_TREE_TEST)
    add_executable(PARSE_TREE_test ParseTree.c)
    target_compile_definitions(PARSE_TREE_test PRIVATE "PARSE_TREE_test")
    target_link_libraries(PARSE_TREE_test PRIVATE sanitizer_flags Utility)
endif ()
