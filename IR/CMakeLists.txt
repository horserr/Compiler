add_library(IR STATIC IR.c Compile.c)
target_include_directories(IR PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(IR PRIVATE ${CMAKE_SOURCE_DIR}/SymbolTable)
target_link_libraries(IR PRIVATE sanitizer_flags Utility)
