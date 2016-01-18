cmake_minimum_required(VERSION 3.3)
project(filtration)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
add_executable(filtration ${SOURCE_FILES})

target_link_libraries(filtration libglog.so)