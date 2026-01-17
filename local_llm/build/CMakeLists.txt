cmake_minimum_required(VERSION 3.18)
project(LocalLLM LANGUAGES CXX C)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/../bin")

# Enable CUDA for llama.cpp
set(GGML_CUDA ON CACHE BOOL "Build with CUDA support" FORCE)

# Set the install location of llama.cpp
set(LLAMA_CPP_INSTALL "${CMAKE_CURRENT_SOURCE_DIR}/../../../llama.cpp")
message("Using llama.cpp install at " ${LLAMA_CPP_INSTALL})

# add llama.cpp
add_subdirectory(${LLAMA_CPP_INSTALL} "${CMAKE_CURRENT_BINARY_DIR}/llama_build")

# Include directories for headers
target_include_directories(llm_test PRIVATE
    "${LLAMA_CPP_INSTALL}/include"
    "${LLAMA_CPP_INSTALL}/src"
    "${CMAKE_SOURCE_DIR}/../include"
)

# add output executable
add_executable(llm_test ../src/main.cpp)

# Link against the llama library
# llama.cpp defines the target 'llama'
target_link_libraries(llm_test PRIVATE 
llama

)

# Set C++ standard (llama.cpp requires C++11 or higher, 17 is recommended)
target_compile_features(llm_test PRIVATE cxx_std_17)
