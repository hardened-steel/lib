file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
list(FILTER SOURCES EXCLUDE REGEX "test.main.cpp")

add_library(${PROJECT_NAME} ${HEADERS} ${SOURCES})
set_target_properties(${PROJECT_NAME}
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS YES
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS YES
)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)

add_subdirectory(common)
add_subdirectory(data-structures)
add_subdirectory(fp)
add_subdirectory(lockfree)
add_subdirectory(platform)
add_subdirectory(typetraits)
add_subdirectory(units)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test-sources/)
add_executable(${PROJECT_NAME}-test ${HEADERS} ${SOURCES} test.main.cpp)
set_target_properties(${PROJECT_NAME}-test
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS YES
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS YES
)
get_target_property(SOURCES ${PROJECT_NAME} SOURCES)
list(FILTER SOURCES INCLUDE REGEX "^.*\.hpp")

foreach(FILE ${SOURCES})
    file(RELATIVE_PATH FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR} ${FILE})
    set(INCLUDE_NAME ${FILE_NAME})
    set(FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/test-sources/${FILE_NAME}.cxx)
    file(WRITE ${FILE_NAME}
        "#include <lib/test.hpp>\n"
        "#include <lib/${INCLUDE_NAME}>\n"
        "\n"
        "\n"
        "namespace {\n"
        "    lib::Test::CaseRegister tests_register = lib::typetraits::get<\"lib-test-cases\", lib::Test>;\n"
        "}\n"
    )
    target_sources(${PROJECT_NAME}-test PUBLIC ${FILE_NAME})
endforeach()

target_include_directories(${PROJECT_NAME}-test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_link_libraries(${PROJECT_NAME}-test PUBLIC ${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME})
