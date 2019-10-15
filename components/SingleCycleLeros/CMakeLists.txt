file(GLOB LIB_SOURCES *.cpp)
file(GLOB LIB_HEADERS *.h)

add_library(SingleCycleLeros ${LIB_SOURCES} ${LIB_HEADERS})
