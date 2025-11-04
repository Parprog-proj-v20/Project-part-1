cmake_minimum_required(VERSION 3.13)
project(Project-part-1)

set(CMAKE_CXX_STANDARD 17)

add_executable(Project-part-1
    src/main.cpp
    src/computerRoom.cpp
)

target_include_directories(Project-part-1 PRIVATE include)

if(MSVC)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

option(BUILD_TESTS "Build tests" ON)
enable_testing()

if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(googletest)
    message(STATUS "GTest loaded, creating tests")
    
    function(add_test_executable test_name)
        add_executable(${test_name} ${ARGN})
        target_include_directories(${test_name} PRIVATE include)
        target_link_libraries(${test_name} PRIVATE gtest gtest_main)
    endfunction()

    add_test_executable(unit_tests tests/unit_tests.cpp src/computerRoom.cpp)
    add_test_executable(integration_tests tests/integration_tests.cpp src/computerRoom.cpp)
    add_test_executable(validation_tests tests/validation_tests.cpp src/computerRoom.cpp)
    add_test_executable(system_tests tests/system_tests.cpp src/computerRoom.cpp)
    add_test_executable(thread_safety_tests tests/thread_safety_tests.cpp src/computerRoom.cpp)

    include(GoogleTest)
    gtest_discover_tests(unit_tests)
    gtest_discover_tests(integration_tests)
    gtest_discover_tests(validation_tests)
    gtest_discover_tests(system_tests)
    gtest_discover_tests(thread_safety_tests)
    
    message(STATUS "Tests created successfully")  
else()
    message(STATUS "Tests disabled")
endif()
