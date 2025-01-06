cmake_minimum_required(VERSION 3.9)


######################################################################
## GUI setup
######################################################################

# Find Qt5Core and Qt5Widgets
find_package(Qt6 COMPONENTS Core Widgets OpenGLWidgets REQUIRED)

# set Qt specific files...
# Would really like to split this up, but Qt coding standards wrt. MOC'd ui
# include files somewhat expect to be nested alongside the source files.
file(GLOB LIB_SOURCES ${VSRTL_INCLUDE_DIR}/VSRTL/graphics/*.cpp)
file ( GLOB UIS ${VSRTL_INCLUDE_DIR}/VSRTL/graphics/*.ui)
file ( GLOB RESOURCES
    ${VSRTL_INCLUDE_DIR}/VSRTL/graphics/resources/icons/*.qrc
    ${VSRTL_INCLUDE_DIR}/VSRTL/graphics/resources/fonts/*.qrc
)

add_library(vsrtl_graphics STATIC ${LIB_SOURCES} ${UIS} ${RESOURCES})
add_library(vsrtl::graphics ALIAS vsrtl_graphics)

# dump ui files
message("UI files: ${UIS}")

option(VSRTL_DEBUG_DRAWING "Draw bounding rectangles & scene grid background" OFF)
if(VSRTL_DEBUG_DRAWING)
    target_compile_definitions(vsrtl_graphics PRIVATE VSRTL_DEBUG_DRAW=1)
endif()

target_link_libraries(vsrtl_graphics
    Qt6::Core
    Qt6::Widgets
    Qt6::OpenGLWidgets
    cereal
    Signals::Signals
    vsrtl::interface
    ${CMAKE_THREAD_LIBS_INIT}
)

# For UI files
message("Adding include directory: ${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(vsrtl_graphics PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(vsrtl_graphics PROPERTIES LINKER_LANGUAGE CXX)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    # https://doc.qt.io/qt-6/wasm.html#asyncify
    target_link_options(vsrtl_graphics PUBLIC -sASYNCIFY -Os)
endif()


if(VSRTL_COVERAGE)
    target_link_libraries(vsrtl_graphics ${COVERAGE_LIB})
endif(VSRTL_COVERAGE)

