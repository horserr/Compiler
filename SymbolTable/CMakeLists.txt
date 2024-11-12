add_library(SymbolTable STATIC
        SymbolTable.c
        Environment.c
        RBTree.c
)
target_include_directories(SymbolTable PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SymbolTable PUBLIC
        PARSE_TREE
        Utility
        sanitizer_flags
)

if (SYMBOL_TEST)
    add_executable(SYMBOL_test SymbolTable.c)
    target_compile_definitions(SYMBOL_test PRIVATE "SYMBOL_test")
    target_link_libraries(SYMBOL_test PRIVATE Environment sanitizer_flags)
endif ()
