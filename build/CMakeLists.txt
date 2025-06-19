cmake_minimum_required(VERSION 3.10) # minimum CMake version
project(testProject VERSION 0.1 LANGUAGES CXX) # CMake project name and version; one of these per CMakeLists.txt

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Generally good practice for portability
set(EXECUTABLE_OUTPUT_PATH )

# ${PROJECT_NAME} gets the project name
# ${PROJECT_SRC_DIR} project source directory
# ${PROJECT_BINARY_DIR} project binary/output directory

add_executable(test ../src/vulkan/vulkan_test.cpp) # create executable from the specified source code files withe the name test

# configure_file()
# message() print ot console
message(${PROJECT_BINARY_DIR})
message(${PROJECT_SOURCE_DIR})

# target_include_directories(${PROJECT_NAME} PUBLIC "${}PROJECT_BINARY_DIR") # PUBLIC means make available for all executables, PRIVATE means just for the sole executable

target_link_libraries(test
    PRIVATE 
        glfw
        vulkan
        dl
        pthread
        X11
        Xxf86vm
)