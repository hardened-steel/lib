add_executable(${PROJECT_NAME}-test
    units.cpp
    #parser.cpp
    typetraits.cpp
    #interpreter.cpp
    channel.cpp
    event.cpp
    cycle.buffer.cpp
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
install(TARGETS ${PROJECT_NAME}-test RUNTIME DESTINATION bin)
