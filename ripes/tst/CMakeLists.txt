cmake_minimum_required(VERSION 3.9)

file(GLOB TST_SOURCES *.cpp)
file(GLOB TST_HEADERS *.h)

add_executable(ripes_tests main.cpp ${TST_SOURCES} ${TST_HEADERS})

target_link_libraries(ripes_tests LINK_PUBLIC Ripes)
