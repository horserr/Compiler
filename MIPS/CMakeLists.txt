add_library(MIPS STATIC Morph.c Optimize.c)
target_include_directories(MIPS PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(MIPS PRIVATE ${CMAKE_SOURCE_DIR}/IR)
target_link_libraries(MIPS PRIVATE sanitizer_flags Utility)
