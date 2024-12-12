add_library(MIPS STATIC Morph.c)
target_include_directories(MIPS PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(MIPS PRIVATE ${CMAKE_SOURCE_DIR}/IR)
target_link_libraries(MIPS PRIVATE sanitizer_flags Utility)

add_library(Optimize STATIC Optimize.c)
target_include_directories(Optimize PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Optimize PRIVATE sanitizer_flags Utility IR)
