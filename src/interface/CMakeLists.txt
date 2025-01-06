cmake_minimum_required(VERSION 3.9)

file(GLOB LIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB LIB_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_library(vsrtl_interface STATIC ${LIB_SOURCES} ${LIB_HEADERS})
add_library(vsrtl::interface ALIAS vsrtl_interface)
target_include_directories (vsrtl_interface PUBLIC
  ${VSRTL_INCLUDE_DIR}
)
set_target_properties(vsrtl_interface PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(vsrtl_interface PUBLIC
  Signals::Signals
  magic_enum::magic_enum
)
