cmake_minimum_required(VERSION 3.9)

file(GLOB LIB_SOURCES *.cpp)
file(GLOB LIB_HEADERS *.h)

add_library(${EDA_LIB} ${LIB_SOURCES} ${LIB_HEADERS} )
target_include_directories (${EDA_LIB} PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories (${EDA_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


######################################################################
## Linking
######################################################################
target_link_libraries(${EDA_LIB} ${GRAPHICS_LIB} Qt5::Core)

include_directories("../core/")

if(COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_libraries(${EDA_LIB} ${COVERAGE_LIB})
endif(COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
