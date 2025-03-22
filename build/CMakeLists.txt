cmake_minimum_required(VERSION 3.10) # minimum CMake version
project(testProject VERSION 0.1) # CMake project name and version

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(test ../src/main.cpp) # create executable