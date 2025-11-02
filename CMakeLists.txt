cmake_minimum_required(VERSION 3.13)

project(Project-part-1)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
    src/main.cpp
    src/computer_room.cpp
)

add_executable(Project-part-1 ${SOURCES})

target_include_directories(Project-part-1
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/include
)
