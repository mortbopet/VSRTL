cmake_minimum_required(VERSION 3.9)

file(GLOB TST_SOURCES *.cpp)

find_package(Qt6 COMPONENTS Test REQUIRED)

macro(create_qtest name)
    add_executable(${name} ${name}.cpp)
    add_test(${name} ${name})
    target_link_libraries(${name} Qt6::Core Qt6::Widgets Qt6::Test)
    target_link_libraries(${name} ${VSRTL_CORE_LIB} ${VSRTL_GRAPHICS_LIB} ${VSRTL_COMPONENTS_LIB})
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
create_qtest(tst_enumandmux)
create_qtest(tst_aluandreg)
create_qtest(tst_registerfile)
create_qtest(tst_memory)
create_qtest(tst_leros)
