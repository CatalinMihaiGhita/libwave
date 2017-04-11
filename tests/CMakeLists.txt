enable_testing()
add_definitions(-std=c++14)
add_executable(tests
    tests.cpp
    ${gtestmain}
)

target_link_libraries(tests
    uv
    gtest
    gmock
)

add_test(
    NAME tests
    COMMAND tests
)
