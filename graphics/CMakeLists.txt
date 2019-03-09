cmake_minimum_required(VERSION 3.9)

file(GLOB LIB_SOURCES *.cpp)
file(GLOB LIB_HEADERS *.h)

######################################################################
## GUI setup
######################################################################

# set Qt specific files
file ( GLOB UIS *.ui)
file ( GLOB RESOURCES resources/*.qrc)

######################################################################
## Linking
######################################################################

add_library(${GRAPHICS_LIB} ${LIB_SOURCES} ${LIB_HEADERS} ${UIS} ${RESOURCES} )
target_link_libraries(${GRAPHICS_LIB} Qt5::Core Qt5::Widgets)

target_include_directories(${GRAPHICS_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include_directories("../core/")

set_target_properties(${GRAPHICS_LIB} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${GRAPHICS_LIB} ${VSRTL_CORE})

if(COVERAGE)
    target_link_libraries(${GRAPHICS_LIB} ${COVERAGE_LIB})
endif(COVERAGE)

