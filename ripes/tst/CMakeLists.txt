cmake_minimum_required(VERSION 3.9)

file(GLOB TST_SOURCES *.cpp)
file(GLOB TST_HEADERS *.h)

add_executable(${TEST_LIB} main.cpp ${TST_SOURCES} ${TST_HEADERS})

INCLUDE_DIRECTORIES("../core/")
INCLUDE_DIRECTORIES("../graphics/")

target_include_directories(${TEST_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${TEST_LIB} ${CORE_LIB} ${GRAPHICS_LIB})

target_link_libraries(${TEST_LIB} Qt5::Core Qt5::Widgets)


