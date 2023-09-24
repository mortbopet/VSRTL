cmake_minimum_required(VERSION 3.9)

file(GLOB LIB_SOURCES *.cpp)
file(GLOB LIB_HEADERS *.h)

######################################################################
## GUI setup
######################################################################

# set Qt specific files
file ( GLOB UIS *.ui)
file ( GLOB RESOURCES resources/icons/*.qrc resources/fonts/*.qrc)

######################################################################
## Linking
######################################################################

add_library(${VSRTL_GRAPHICS_LIB} STATIC ${LIB_SOURCES} ${LIB_HEADERS} ${UIS} ${RESOURCES} )

option(VSRTL_DEBUG_DRAWING "Draw bounding rectangles & scene grid background" OFF)
if(VSRTL_DEBUG_DRAWING)
    target_compile_definitions(${VSRTL_GRAPHICS_LIB} PRIVATE VSRTL_DEBUG_DRAW=1)
endif()

find_package(Qt6 REQUIRED COMPONENTS OpenGLWidgets)
target_link_libraries(${VSRTL_GRAPHICS_LIB} Qt6::Core Qt6::Widgets Qt6::OpenGLWidgets)

target_include_directories(${VSRTL_GRAPHICS_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include_directories("../core/")

set_target_properties(${VSRTL_GRAPHICS_LIB} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${VSRTL_GRAPHICS_LIB} ${VSRTL_CORE} ${VSRTL_INTERFACE_LIB} ${CMAKE_THREAD_LIBS_INIT})
if(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    # https://doc.qt.io/qt-6/wasm.html#asyncify
    target_link_options(${VSRTL_GRAPHICS_LIB} PUBLIC -sASYNCIFY -Os)
endif()


if(VSRTL_COVERAGE)
    target_link_libraries(${VSRTL_GRAPHICS_LIB} ${COVERAGE_LIB})
endif(VSRTL_COVERAGE)

