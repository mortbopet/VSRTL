cmake_minimum_required(VERSION 3.13)
set (CMAKE_CXX_STANDARD 17)
project(vsrtl CXX)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "-Wextra -Wall -Wno-unused-local-typedefs -Wno-unused-function")
endif()

######################################################################
## GUI setup
######################################################################
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
find_package(Threads REQUIRED)

######################################################################
## Fetch dependencies
######################################################################

include(FetchContent)

# set CMP0077 for ancient cereal version to comply with JUST_INSTALL_CEREAL
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(JUST_INSTALL_CEREAL ON)
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

# Better enum
FetchContent_Declare(
  magic-enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG v0.9.7
)

FetchContent_MakeAvailable(magic-enum)

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
set(VSRTL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_subdirectory(src)

# Top-level library that pulls in everything
add_library(vsrtl_lib INTERFACE)
target_link_libraries(vsrtl_lib INTERFACE
    vsrtl::interface
    vsrtl::graphics
)
add_library(vsrtl::vsrtl ALIAS vsrtl_lib)

option(VSRTL_BUILD_TESTS "Build the VSRTL test suite" ON)
if(VSRTL_BUILD_TESTS)
    set(VSRTL_TEST_LIB ${PROJECT_NAME}_test_lib CACHE INTERNAL "")
    add_subdirectory(test)
endif()

option(VSRTL_BUILD_APP "Build the VSRTL standalone application" ON)
if(VSRTL_BUILD_APP)
    set(APP_NAME VSRTL)
    add_executable(${APP_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/app.cpp)
    target_link_libraries(${APP_NAME}
      vsrtl::vsrtl
    )
endif()
