cmake_minimum_required(VERSION 3.10)

project(FutureTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR})

find_package(Catch2 REQUIRED CONFIG)
include_directories(${Catch2_INCLUDE_DIRS})

add_executable(tests
    ${CMAKE_SOURCE_DIR}/tests/unit.cpp
    ${CMAKE_SOURCE_DIR}/tests/stress.cpp
)

target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

enable_testing()
add_test(NAME FutureTests COMMAND tests)