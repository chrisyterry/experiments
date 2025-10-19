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

# get the slang compiler
find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin REQUIRED)

# Device selection
add_library(device_selection STATIC
    ${CMAKE_SOURCE_DIR}/../src/vulkan/device_utils.cpp
)
target_include_directories(device_selection PUBLIC ${CMAKE_SOURCE_DIR}/../include)
target_link_libraries(device_selection
    Vulkan::cppm
    glfw
)

# pipeline creation
add_library(pipeline_creation STATIC
    ${CMAKE_SOURCE_DIR}/../src/vulkan/pipeline_utils.cpp
)
target_include_directories(pipeline_creation PUBLIC ${CMAKE_SOURCE_DIR}/../include)
target_link_libraries(pipeline_creation
    Vulkan::cppm
    device_selection
)

# Renderer
add_library(renderer STATIC
    ${CMAKE_SOURCE_DIR}/../src/vulkan/modern_render_triangle.cpp
)
target_include_directories(renderer PUBLIC ${CMAKE_SOURCE_DIR}/../include)
target_link_libraries(renderer
    Vulkan::cppm
    glfw
    device_selection
    pipeline_creation
)
add_executable(nu_triangle ../src/vulkan/main.cpp)
target_include_directories(nu_triangle PUBLIC ${CMAKE_SOURCE_DIR}/../include)
target_link_libraries(nu_triangle
    renderer
)

# slang shader attachment
function (add_slang_shader_target TARGET)
    # SHADER_SOURCES  contains shader files to use
    cmake_parse_arguments("SHADER" "" "" "SOURCES" ${ARGN})
    set (SPV_NAME nu_triangle_shaders)
    set (SHADERS_DIR ${EXECUTABLE_OUTPUT_PATH}/${TARGET})
    set (ENTRY_POINTS -entry vertMain -entry fragMain)
    set (OUTPUT_SPV ${SHADERS_DIR}/${SPV_NAME}.spv)
    add_custom_command(
        OUTPUT ${SHADERS_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
        VERBATIM
    )
    add_custom_command(
        OUTPUT ${OUTPUT_SPV}
        COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES} 
            -target spirv 
            -profile spirv_1_4 
            -emit-spirv-directly 
            -fvk-use-entrypoint-name ${ENTRY_POINTS} 
            -o ${SPV_NAME}.spv
        WORKING_DIRECTORY ${SHADERS_DIR}
        DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
        COMMENT "Compiling slang shaders ${SHADER_SOURCES} to ${OUTPUT_SPV_FILE}.slv"
        VERBATIM
    )
    add_custom_target(${TARGET}
        DEPENDS ${SHADERS_DIR}/${SPV_NAME}.spv
    )
endfunction()

set(SLANG_SHADERS 
    ${CMAKE_SOURCE_DIR}/../shaders/nu_triangle.slang
)
add_slang_shader_target(shaders SOURCES ${SLANG_SHADERS}) # add slang shader target
add_dependencies(nu_triangle shaders)

if (DEFINED CHAPTER_SHADER)
    set (CHAPTER_SHADER_TARGET ${CHAPTER_NAME}_shader)
    file (GLOB SHADER_SOURCES ${CHAPTER_SHADER}.frag ${CHAPTER_SHADER}.vert ${CHAPTER_SHADER}.comp)
    add_shaders_target (${CHAPTER_SHADER_TARGET} CHAPTER_NAME ${CHAPTER_NAME} SOURCES ${SHADER_SOURCES})
    add_dependencies (${CHAPTER_NAME} ${CHAPTER_SHADER_TARGET})

    set (CHAPTER_SHADER_SLANG_TARGET ${CHAPTER_NAME}_slang_shader)
    file (GLOB SHADER_SLANG_SOURCES ${CHAPTER_SHADER}.slang)
    if(SHADER_SLANG_SOURCES)
      add_slang_shader_target( ${CHAPTER_SHADER_SLANG_TARGET} CHAPTER_NAME ${CHAPTER_NAME} SOURCES ${SHADER_SLANG_SOURCES})
      add_dependencies(${CHAPTER_NAME} ${CHAPTER_SHADER_SLANG_TARGET})
    endif()
  endif ()

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
