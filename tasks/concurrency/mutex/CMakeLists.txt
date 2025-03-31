cmake_minimum_required(VERSION 3.10)

# Define the project
project(MutexTests LANGUAGES CXX)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Catch2
find_package(Catch2 REQUIRED CONFIG)
include_directories(${Catch2_INCLUDE_DIRS})

# Ensure correct paths for source and header files
set(SRC_DIR ${CMAKE_SOURCE_DIR})
set(INCLUDE_DIR ${SRC_DIR})  # Assuming mutex.hpp is in the source folder

# Explicitly include header directory
include_directories(${INCLUDE_DIR})

# Add test executable
add_executable(tests
    ${SRC_DIR}/tests/unit.cpp
    ${SRC_DIR}/tests/stress.cpp
)

# Link Catch2
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

# Enable testing
enable_testing()
add_test(NAME MutexTests COMMAND tests)
