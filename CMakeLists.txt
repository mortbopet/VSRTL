cmake_minimum_required(VERSION 3.9)
set (CMAKE_CXX_STANDARD 17)
project(vsrtl)

######################################################################
## GUI setup
######################################################################

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt5Core and Qt5Widgets
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets CONFIG REQUIRED)

# Finding Qt includes
include_directories(${Qt5Widgets_INCLUDE_DIRS})
include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(external)

######################################################################

set(CORE_LIB ${PROJECT_NAME}_core_lib)
add_subdirectory(core)
set(GRAPHICS_LIB ${PROJECT_NAME}_gfx_lib)
add_subdirectory(graphics)
set(TEST_LIB ${PROJECT_NAME}_tst_lib)
add_subdirectory(tst)
set(COMPONENTS_LIB ${PROJECT_NAME}_cmp_lib)
add_subdirectory(components)


set(APP_NAME app)

add_executable(${APP_NAME} app.cpp)
target_link_libraries(${APP_NAME} Qt5::Core Qt5::Widgets)
target_link_libraries(${APP_NAME} ${CORE_LIB} ${GRAPHICS_LIB} ${COMPONENTS_LIB})
