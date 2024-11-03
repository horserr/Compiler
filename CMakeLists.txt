cmake_minimum_required(VERSION 3.20)
project(Compiler LANGUAGES C)
set(CMAKE_C_STANDARD 11)

option(PARSE_TREE_TEST "test PARSE TREE date structure" OFF)
option(PARSER_DEBUG "debug parser with output file and debug mode being set" OFF)
option(RBTREE_TEST "test red black tree" ON)
option(SYMBOL_TEST "test symbol table" OFF)
ADD_COMPILE_DEFINITIONS("LOCAL")

# todo add sanitizer flags in debug mode and git
# Set sanitizer flags
set(SANITIZER_FLAGS "-fsanitize=address")
set(DEBUG_FLAGS "-g")
# Define ASAN options as a list
set(ASAN_OPTIONS_LIST
        "detect_leaks=1"
        "detect_stack_use_after_return=1"
        "detect_stack_use_after_scope=1"
        "halt_on_error=1"
        "verbosity=1"
        "malloc_context_size=4"
        # "detect_invalid_pointer_pairs=1"
)

# Add subdirectories
add_subdirectory(ParseTree)
add_subdirectory(SymbolTable)

# Create a library with sanitizer flags
add_library(sanitizer_flags INTERFACE)
target_compile_options(sanitizer_flags INTERFACE ${SANITIZER_FLAGS} ${DEBUG_FLAGS})
target_link_libraries(sanitizer_flags INTERFACE ${SANITIZER_FLAGS})
# Turn option list into a single string
message(STATUS "ASAN_OPTIONS_LIST is ${ASAN_OPTIONS_LIST}")
list(JOIN ASAN_OPTIONS_LIST ":" ASAN_OPTIONS)
message(STATUS "ASAN_OPTIONS is ${ASAN_OPTIONS}")
# Set sanitizer environment variables
set_target_properties(sanitizer_flags PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "ASAN_OPTIONS=${ASAN_OPTIONS}")
# todo link this library to other targets

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
target_link_libraries(${PROJECT_NAME}
        PARSE_TREE SymbolTable
        ${FLEX_LIBRARIES}
        ${BISON_LIBRARIES}
)

