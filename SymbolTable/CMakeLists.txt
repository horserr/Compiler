add_library(RBTree STATIC RBTree.c)
target_include_directories(RBTree PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(RBTree PUBLIC PARSE_TREE Utility)

add_library(Environment STATIC Environment.c)
target_include_directories(Environment PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Environment PUBLIC RBTree)

add_library(SymbolTable STATIC SymbolTable.c)
target_include_directories(SymbolTable PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SymbolTable PUBLIC Environment)

# soft link
#execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/SymbolTable.h ${CMAKE_CURRENT_BINARY_DIR}/SymbolTable.h)


if (RBTREE_TEST)
    add_executable(RBTREE_test RBTree.c)
    target_compile_definitions(RBTREE_test PRIVATE "RBTREE_test")
    target_link_libraries(RBTREE_test PRIVATE PARSE_TREE sanitizer_flags)
endif ()

if (SYMBOL_TEST)
    add_executable(SYMBOL_test SymbolTable.c)
    target_compile_definitions(SYMBOL_test PRIVATE "SYMBOL_test")
    target_link_libraries(SYMBOL_test PRIVATE Environment sanitizer_flags)
endif ()
