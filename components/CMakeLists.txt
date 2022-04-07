cmake_minimum_required(VERSION 3.9)

file(GLOB LIB_SOURCES *.cpp)
file(GLOB LIB_HEADERS *.h)

######################################################################
## Linking
######################################################################

INCLUDE_DIRECTORIES("../core/")

add_library(${VSRTL_COMPONENTS_LIB} STATIC ${LIB_SOURCES} ${LIB_HEADERS})

set_target_properties(${VSRTL_COMPONENTS_LIB} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${VSRTL_COMPONENTS_LIB} ${VSRTL_CORE})

add_subdirectory(Leros)
