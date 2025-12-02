file(GLOB SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.hpp)

target_sources(lib PUBLIC ${HEADERS} ${SOURCES})
