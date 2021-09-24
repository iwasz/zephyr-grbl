CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT (unit-tests)

add_subdirectory(Catch2)
add_executable(tests 00regexps.cc)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

include_directories(../../deps/compile-time-regular-expressions/include)

SET(CMAKE_C_FLAGS "-std=gnu99 -Wall" CACHE INTERNAL "c compiler flags")
SET(CMAKE_CXX_FLAGS "-std=c++20 -Wall" CACHE INTERNAL "cxx compiler flags")


