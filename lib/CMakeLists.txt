set(SOURCES
    event.cpp
    channel.cpp
)

add_library(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)
install(TARGETS ${PROJECT_NAME})
