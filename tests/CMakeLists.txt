add_definitions(-std=c++14)
add_executable(tests gtest_main.cc tests.cpp)

target_link_libraries(tests
    uv
    gtest
    gmock
)

install(TARGETS tests DESTINATION bin)
