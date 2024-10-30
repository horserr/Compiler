add_library(SymbolTable STATIC SymbolTable.c)
target_include_directories(SymbolTable PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SymbolTable PRIVATE PARSE_TREE)

# soft link
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/SymbolTable.h ${CMAKE_CURRENT_BINARY_DIR}/SymbolTable.h)

if (SYMBOL_TEST)
    add_executable(SYMBOL_test SymbolTable.c)
    target_compile_definitions(SYMBOL_test PRIVATE "SYMBOL_test")
endif ()
