cmake_minimum_required(VERSION 3.9)

file(GLOB TST_SOURCES *.cpp)

find_package(Qt6 COMPONENTS Test REQUIRED)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

macro(create_qtest name)
    add_executable(${name} ${name}.cpp)
    add_test(${name} ${name})
    target_link_libraries(${name}
        Qt6::Test
        vsrtl::vsrtl
    )
    target_link_libraries(${name} vsrtl::vsrtl)
    if (COVERAGE)
        target_link_libraries(${name} ${COVERAGE_LIB})
    endif()
endmacro()

create_qtest(tst_adderandreg)
create_qtest(tst_nestedcomponent)
create_qtest(tst_counter)
create_qtest(tst_rannumgen)
create_qtest(tst_enumandmux)
create_qtest(tst_aluandreg)
create_qtest(tst_registerfile)
create_qtest(tst_memory)
create_qtest(tst_leros)
