cmake_minimum_required(VERSION 3.13)
project(Project-part-1)
set(CMAKE_CXX_STANDARD 17)

add_executable(Project-part-1 src/main.cpp src/computer_room.cpp)
target_include_directories(Project-part-1 PRIVATE include)

if(MSVC)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(googletest)
    message(STATUS "GTest loaded, creating tests")
    
    add_executable(unit_tests tests/unit_tests.cpp src/computer_room.cpp)
    target_include_directories(unit_tests PRIVATE include)
    target_link_libraries(unit_tests gtest gtest_main)

    add_executable(integration_tests tests/integration_tests.cpp src/computer_room.cpp)
    target_include_directories(integration_tests PRIVATE include)
    target_link_libraries(integration_tests gtest gtest_main)

    add_executable(validation_tests tests/validation_tests.cpp src/computer_room.cpp)
    target_include_directories(validation_tests PRIVATE include)
    target_link_libraries(validation_tests gtest gtest_main)
    
    add_executable(system_tests tests/system_tests.cpp src/computer_room.cpp)
    target_include_directories(system_tests PRIVATE include)
    target_link_libraries(system_tests gtest gtest_main)

    add_executable(thread_safety_tests tests/thread_safety_tests.cpp src/computer_room.cpp)
    target_include_directories(thread_safety_tests PRIVATE include)
    target_link_libraries(thread_safety_tests gtest gtest_main)

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