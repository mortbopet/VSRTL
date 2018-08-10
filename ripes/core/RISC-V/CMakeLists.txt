cmake_minimum_required(VERSION 3.9)

set(RISCV_LIB ${PROJECT_NAME}_riscv_lib)

file(GLOB LIB_SOURCES *.cpp)
file(GLOB LIB_HEADERS *.h)

add_library(${RISCV_LIB} ${LIB_SOURCES} ${LIB_HEADERS} )
target_include_directories(${RISCV_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(${RISCV_LIB} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${RISCV_LIB} LINK_PUBLIC ${CORE_LIB})


