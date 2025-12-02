cmake_minimum_required(VERSION 3.6)
project(lib LANGUAGES CXX C VERSION 0.0.1)

LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(FetchContent)
include(platform)
enable_testing()

add_subdirectory(lib)

include(CPack)
