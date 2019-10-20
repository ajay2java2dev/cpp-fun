cmake_minimum_required(VERSION 3.15)
project(AircraftProject)

set(CMAKE_CXX_STANDARD 14)

add_executable(AircraftProject main.cpp Aircraft.h Aircraft.cpp Fighter.h Fighter.cpp)