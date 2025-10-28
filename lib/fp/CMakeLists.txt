file(GLOB SOURCES *.cpp)
file(GLOB HEADERS *.hpp)
file(GLOB HEADERS details/*.hpp)

target_sources(lib PUBLIC ${HEADERS} ${SOURCES})
