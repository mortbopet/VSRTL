cmake_minimum_required(VERSION 3.9)

add_subdirectory(SingleCycleLeros)
target_link_libraries(SingleCycleLeros ${VSRTL_CORE})
