set (CMAKE_CXX_STANDARD 14)
project(Ripes)

file(GLOB LIB_SOURCES src/*.cpp)
file(GLOB LIB_HEADERS src/*.h)

file(GLOB TST_SOURCES tst/*.cpp)
file(GLOB TST_HEADERS tst/*.h)

set(TEST_EXECUTABLE ${PROJECT_NAME}_tst)

add_library(${PROJECT_NAME} ${LIB_SOURCES} ${LIB_HEADERS} )
add_executable(${TEST_EXECUTABLE} main.cpp ${TST_SOURCES} ${TST_HEADERS})

target_include_directories(${PROJECT_NAME}_tst PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(${TEST_EXECUTABLE} ${PROJECT_NAME})




