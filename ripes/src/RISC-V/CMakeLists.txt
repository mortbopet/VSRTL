cmake_minimum_required(VERSION 3.9)

file(GLOB LIB_SOURCES *.cpp)
file(GLOB LIB_HEADERS *.h)

add_library(Ripes_RISCV ${LIB_SOURCES} ${LIB_HEADERS} )
target_include_directories(Ripes_RISCV PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(Ripes_RISCV PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(Ripes_RISCV LINK_PUBLIC Ripes)


