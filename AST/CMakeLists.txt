add_library(AST STATIC AST.c)
target_include_directories(AST PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# soft link
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/AST.h ${CMAKE_CURRENT_BINARY_DIR}/AST.h)

if (AST_TEST)
    add_executable(AST_test AST.c)
    target_compile_definitions(AST_test PRIVATE "AST_test")
endif ()
