cmake_minimum_required(VERSION 3.20)
project(Compiler LANGUAGES C)
set(CMAKE_C_STANDARD 11)

option(AST_TEST "test AST date structure" OFF)
option(PARSER_DEBUG "debug parser with output file and debug mode being set" OFF)
ADD_COMPILE_DEFINITIONS("LOCAL")

add_subdirectory(AST)

find_package(FLEX 2.6.4 REQUIRED)
find_package(BISON 3.0.4 REQUIRED)

flex_target(Lexer lexical.l ${CMAKE_BINARY_DIR}/lex.yy.c
        DEFINES_FILE ${CMAKE_BINARY_DIR}/lex.yy.h
)
if (PARSER_DEBUG)
    add_compile_definitions("PARSER_DEBUG")
    bison_target(Parser syntax.y ${CMAKE_BINARY_DIR}/syntax.tab.c
            DEFINES_FILE ${CMAKE_BINARY_DIR}/syntax.tab.h
            COMPILE_FLAGS "-v"
    )
else ()
    bison_target(Parser syntax.y ${CMAKE_BINARY_DIR}/syntax.tab.c
            DEFINES_FILE ${CMAKE_BINARY_DIR}/syntax.tab.h
    )
endif ()
add_flex_bison_dependency(Lexer Parser)
add_executable(${PROJECT_NAME}
        main.c
        ${FLEX_Lexer_OUTPUTS}
        ${BISON_Parser_OUTPUTS}
)
#target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/AST)
target_link_libraries(${PROJECT_NAME}
        AST
        ${FLEX_LIBRARIES}
        ${BISON_LIBRARIES}
)

#include(CTest)
#add_test(NAME simple_test
#        COMMAND ${CMAKE_BINARY_DIR}/Compiler test/test.cmm
#)
