cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

project(pthread_example CXX)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE main.cpp)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
