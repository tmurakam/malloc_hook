cmake_minimum_required(VERSION 3.14)
project(malloc_hook VERSION 0.0.3)

set(CMAKE_CXX_STANDARD 14)

# library
add_library(malloc_hook SHARED
        malloc_hook.c
        mtrace.c
)
target_link_libraries(malloc_hook dl)

# google test
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)
#FetchContent_GetProperties(googletest)

enable_testing()

# test program
add_executable(
        malloc_hook_test
        tests/malloc_hook_test.cpp
        tests/mtrace_test.cpp
)
target_link_libraries(
        malloc_hook_test
        malloc_hook
        GTest::gtest_main
)

include(GoogleTest)
gtest_add_tests(TARGET malloc_hook_test)
#gtest_discover_tests(malloc_hook_test)
