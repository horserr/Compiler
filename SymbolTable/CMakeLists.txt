add_library(SymbolTable STATIC
            SymbolTable.c
            Environment.c
            RBTree.c
            )
target_include_directories(SymbolTable PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(SymbolTable PRIVATE ${CMAKE_SOURCE_DIR}/ParseTree)
target_link_libraries(SymbolTable PRIVATE Utility sanitizer_flags)
