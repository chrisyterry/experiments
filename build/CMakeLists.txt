cmake_minimum_required(VERSION 3.29)
project(VulkanTutorial VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/../bin)

# Enable module scanning and define module-specific flags
# These lines MUST come after the 'project' command
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)

add_library(VulkanCppModule)
add_library(Vulkan::cppm ALIAS VulkanCppModule)

target_compile_definitions(VulkanCppModule PUBLIC
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)

# can disable C++ exceptions using VULKAN_HPP_NO_EXCEPTIONS=1

target_include_directories(VulkanCppModule
    PRIVATE
    "${Vulkan_INCLUDE_DIR}"
)

message("${Vulkan_INCLUDE_DIR}")

target_link_libraries(VulkanCppModule PUBLIC Vulkan::Vulkan)

target_sources(VulkanCppModule
    PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES
    BASE_DIRS "${Vulkan_INCLUDE_DIR}"
    FILES "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
)

add_executable(nu_triangle ../src/vulkan/modern_render_triangle.cpp)

target_include_directories(nu_triangle PUBLIC ${CMAKE_SOURCE_DIR}/../include)

target_link_libraries(nu_triangle
    PRIVATE
    Vulkan::cppm
    glfw
)
#[[

## Original ##

# set the CMake source directory to the overarching directory and the executable directory to the bin folder to make bins easy to find
set(CMAKE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/..)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

include_directories(${CMAKE_SOURCE_DIR}/include)

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

add_executable(render_triangle ../src/vulkan/render_triangle.cpp) # create executable from the specified source code files with the name render_triangle

#target_include_directories(render_triangle PRIVATE directory) # target-specific include


# link the vulkan libraries
target_link_libraries(render_triangle
    PRIVATE 
        glfw
        vulkan
        dl
        pthread
        X11
        Xxf86vm
)

]]
