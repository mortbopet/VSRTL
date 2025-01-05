file(GLOB LIB_SOURCES *.cpp)
file(GLOB LIB_HEADERS *.h)

add_library(SingleCycleLeros STATIC ${LIB_SOURCES} ${LIB_HEADERS})
