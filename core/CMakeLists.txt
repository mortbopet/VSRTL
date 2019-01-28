cmake_minimum_required(VERSION 3.9)

file(GLOB LIB_SOURCES *.cpp)
file(GLOB LIB_HEADERS *.h)

add_library(${CORE_LIB} ${LIB_SOURCES} ${LIB_HEADERS} )
target_include_directories (${CORE_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Architecture specific subdirectories
add_subdirectory(RISC-V)

