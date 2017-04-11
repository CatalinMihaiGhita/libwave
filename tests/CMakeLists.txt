add_executable(tests gtest_main.cc tests.cpp)

target_link_libraries(tests
    libuv
    libgtest
    libgmock
)

target_link_libraries(tests advapi32 iphlpapi psapi userenv shell32 ws2_32)

install(TARGETS tests DESTINATION bin)
