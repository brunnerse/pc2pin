cmake_minimum_required(VERSION 3.15)

SET (CMAKE_CXX_COMPILER             "g++")
SET (CMAKE_CXX_FLAGS                "-Wall")

project(Pin2Serial VERSION 1.0)


add_subdirectory(SerialCpp)


add_executable(Pin2Serial pin2serial.cpp)

target_link_libraries(Pin2Serial SerialCpp)


