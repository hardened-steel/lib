cmake_minimum_required(VERSION 3.18)
project(lib LANGUAGES CXX C)

include(FetchContent)
enable_testing()

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip
)
FetchContent_MakeAvailable(googletest)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
    #units.cpp
)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../)

add_executable(${PROJECT_NAME}-test
    #test/test.cpp
    test/units.cpp
)
set_target_properties(${PROJECT_NAME}-test
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS YES
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS YES
)
target_link_libraries(${PROJECT_NAME}-test PUBLIC ${PROJECT_NAME} gtest_main)
add_test(NAME LibUnits COMMAND ${PROJECT_NAME}-test)
