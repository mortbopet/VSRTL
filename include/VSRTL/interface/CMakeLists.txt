cmake_minimum_required(VERSION 3.9)

file(GLOB LIB_SOURCES *.cpp)
file(GLOB LIB_HEADERS *.h)

add_library(${VSRTL_INTERFACE_LIB} STATIC ${LIB_SOURCES} ${LIB_HEADERS})
target_include_directories (${VSRTL_INTERFACE_LIB} PUBLIC
  ${VSRTL_INCLUDE_DIR}
)
set_target_properties(${VSRTL_INTERFACE_LIB} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${VSRTL_INTERFACE_LIB} PUBLIC
  Signals::Signals
  magic_enum::magic_enum
)
