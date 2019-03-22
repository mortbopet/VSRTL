cmake_minimum_required(VERSION 3.9)

file(GLOB TST_SOURCES *.cpp)

find_package(Qt5Test REQUIRED)

macro(create_qtest name)
    add_executable(${name} ${name}.cpp)
    add_test(${name} ${name})
    target_link_libraries(${name} Qt5::Core Qt5::Widgets Qt5::Test)
    target_link_libraries(${name} ${CORE_LIB} ${GRAPHICS_LIB} ${COMPONENTS_LIB})
    if (COVERAGE)
        target_link_libraries(${name} ${COVERAGE_LIB})
    endif()
endmacro()

INCLUDE_DIRECTORIES("../core/")
INCLUDE_DIRECTORIES("../graphics/")
INCLUDE_DIRECTORIES("../components/")



create_qtest(tst_adderandreg)
create_qtest(tst_nestedcomponent)
create_qtest(tst_counter)
create_qtest(tst_rannumgen)
create_qtest(tst_routingregion)
