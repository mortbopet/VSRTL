cmake_minimum_required(VERSION 3.9)

file(GLOB LIB_SOURCES *.cpp)
file(GLOB LIB_HEADERS *.h)

add_library(${VSRTL_CORE_LIB} STATIC ${LIB_SOURCES} ${LIB_HEADERS} )
target_include_directories (${VSRTL_CORE_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    # https://doc.qt.io/qt-6/wasm.html#asyncify
    target_link_options(${VSRTL_CORE_LIB} PUBLIC -sASYNCIFY -Os)
endif()

if(VSRTL_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_libraries(${VSRTL_CORE_LIB} ${COVERAGE_LIB} ${VSRTL_INTERFACE_LIB})
endif(VSRTL_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
