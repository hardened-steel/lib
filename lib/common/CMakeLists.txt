file(GLOB SOURCES *.cpp)
file(GLOB HEADERS *.hpp)

target_sources(lib PUBLIC ${HEADERS} ${SOURCES})
