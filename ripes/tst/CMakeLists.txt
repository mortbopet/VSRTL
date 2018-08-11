cmake_minimum_required(VERSION 3.9)

file(GLOB TST_SOURCES *.cpp)

find_package(Qt5Test REQUIRED)

macro(create_qtest name)
    add_executable(${name} ${name}.cpp)
    add_test(${name} ${name})
    target_link_libraries(${name} Qt5::Core Qt5::Widgets Qt5::Test)
    target_link_libraries(${name} ${CORE_LIB} ${GRAPHICS_LIB})
endmacro()

INCLUDE_DIRECTORIES("../core/")
INCLUDE_DIRECTORIES("../graphics/")


create_qtest(tst_adderandreg)
create_qtest(tst_addop)
create_qtest(tst_mainwindow)
create_qtest(tst_nestedcomponent)


