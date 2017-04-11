cmake_minimum_required(VERSION 3.0.2)
project(libwave)
include_directories(include)

add_subdirectory(googletest)
include_directories(googletest/googletest/include)
include_directories(googletest/googlemock/include)
file(GLOB gtestmain googletest/googletest/src/gtest_main.cc)

add_subdirectory(libuv)
include_directories(libuv/libuv/include)

add_subdirectory(src)
add_subdirectory(tests)
