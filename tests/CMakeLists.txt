add_definitions(-std=c++14)
add_executable(tests gtest_main.cc tests.cpp)

target_link_libraries(tests
    uv
    gtest
    gmock
)

target_link_libraries(tests advapi32 iphlpapi psapi userenv shell32 ws2_32)

install(TARGETS tests DESTINATION bin)
