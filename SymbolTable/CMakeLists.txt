add_library(SymbolTable STATIC SymbolTable.c)
target_include_directories(SymbolTable PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# soft link
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/SymbolTable.h ${CMAKE_CURRENT_BINARY_DIR}/SymbolTable.h)
