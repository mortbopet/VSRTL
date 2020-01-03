cmake_minimum_required(VERSION 3.9)

file(GLOB LIB_SOURCES *.cpp)
file(GLOB LIB_HEADERS *.h)

add_library(${VSRTL_INTERFACE_LIB} ${LIB_SOURCES} ${LIB_HEADERS})
target_include_directories (${VSRTL_INTERFACE_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(${VSRTL_INTERFACE_LIB} PROPERTIES LINKER_LANGUAGE CXX)
