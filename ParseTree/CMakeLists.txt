add_library(PARSE_TREE STATIC ParseTree.c)
target_include_directories(PARSE_TREE PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# soft link
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/ParseTree.h ${CMAKE_CURRENT_BINARY_DIR}/ParseTree.h)

if (PARSE_TREE_TEST)
    add_executable(PARSE_TREE_test ParseTree.c)
    target_compile_definitions(PARSE_TREE_test PRIVATE "PARSE_TREE_test")
endif ()
