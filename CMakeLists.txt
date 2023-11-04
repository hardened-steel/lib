cmake_minimum_required(VERSION 3.16)
project(lib LANGUAGES CXX C VERSION 0.0.1)

include(FetchContent)
enable_testing()

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(googletest)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(lib)
add_subdirectory(test)

include(CPack)
