cmake_minimum_required(VERSION 3.9)

file(GLOB LIB_SOURCES *.cpp)
file(GLOB LIB_HEADERS *.h)

add_library(${VSRTL_VLT_LIB} ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries(${VSRTL_VLT_LIB} pugixml)
target_include_directories (${VSRTL_VLT_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(VSRTL_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_libraries(${VSRTL_VLT_LIB} ${COVERAGE_LIB} ${VSRTL_INTERFACE_LIB})
endif(VSRTL_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
