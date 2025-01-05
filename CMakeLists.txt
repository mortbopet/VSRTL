cmake_minimum_required(VERSION 3.9)
set (CMAKE_CXX_STANDARD 17)
project(vsrtl CXX)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "-Wextra -Wall -Wno-unused-local-typedefs -Wno-unused-function")
endif()

######################################################################
## GUI setup
######################################################################

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt5Core and Qt5Widgets
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)
find_package(Threads REQUIRED)

######################################################################
## Fetch dependencies
######################################################################

include(FetchContent)
FetchContent_Declare(
    cereal
    GIT_REPOSITORY https://github.com/USCiLab/cereal.git
    GIT_TAG 51cbda5f30e56c801c07fe3d3aba5d7fb9e6cca4
)
FetchContent_MakeAvailable(cereal)

FetchContent_Declare(
  signals
  GIT_REPOSITORY https://github.com/pbhogan/Signals.git
  GIT_TAG 17881fb92ec314bb43549cc2cc8acfac24916f91
)
    
# Signals doesn't have a CMake file, so we download the source files and
# create the library manually
FetchContent_GetProperties(signals)
if(NOT signals_POPULATED)
  FetchContent_Populate(signals)
endif()
add_library(Signals INTERFACE)
add_library(Signals::Signals ALIAS Signals)
target_include_directories(Signals INTERFACE ${signals_SOURCE_DIR})

######################################################################
## Code Coverage Configuration
######################################################################

option(VSRTL_COVERAGE "Enable coverage reporting" OFF)
if(VSRTL_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_library(coverage_config INTERFACE)
  # Add required flags (GCC & LLVM/Clang)
  set(COVERAGE_LIB coverage_config)
  target_compile_options(${COVERAGE_LIB} INTERFACE
    -O0        # no optimization
    -g         # generate debug info
    --coverage # sets all required flags
  )
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
    target_link_options(coverage_config INTERFACE --coverage)
  else()
    target_link_libraries(coverage_config INTERFACE --coverage)
  endif()
endif(VSRTL_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")

######################################################################
## Library setup
######################################################################
set(VSRTL_INTERFACE_LIB ${PROJECT_NAME}_int_lib CACHE INTERNAL "")
add_subdirectory(include/VSRTL/interface)
set(VSRTL_CORE_LIB ${PROJECT_NAME}_core_lib CACHE INTERNAL "")
add_subdirectory(include/VSRTL/core)
set(VSRTL_GRAPHICS_LIB ${PROJECT_NAME}_gfx_lib CACHE INTERNAL "")
add_subdirectory(include/VSRTL/graphics)
set(VSRTL_COMPONENTS_LIB ${PROJECT_NAME}_cmp_lib CACHE INTERNAL "")
add_subdirectory(include/VSRTL/components)

option(VSRTL_BUILD_TESTS "Build the VSRTL test suite" ON)
if(VSRTL_BUILD_TESTS)
    set(VSRTL_TEST_LIB ${PROJECT_NAME}_test_lib CACHE INTERNAL "")
    add_subdirectory(test)
endif()

option(VSRTL_BUILD_APP "Build the VSRTL standalone application" ON)
if(VSRTL_BUILD_APP)
    set(APP_NAME VSRTL)
    add_executable(${APP_NAME} app.cpp)
    target_link_libraries(${APP_NAME} Qt6::Core Qt6::Widgets)
    target_link_libraries(${APP_NAME} ${VSRTL_CORE_LIB} ${VSRTL_GRAPHICS_LIB} ${VSRTL_COMPONENTS_LIB})
endif()
