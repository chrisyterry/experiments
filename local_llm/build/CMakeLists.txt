cmake_minimum_required(VERSION 3.18)
project(LocalLLM LANGUAGES CXX C)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/../bin")

set(LLAMA_CPP_INSTALL "${CMAKE_CURRENT_SOURCE_DIR}/../../../llama.cpp")

# import pre-built ggml-base library
add_library(ggml_base SHARED IMPORTED)
set_target_properties(ggml_base PROPERTIES
    IMPORTED_LOCATION "${LLAMA_CPP_INSTALL}/build_release/bin/libggml-base.so"
)

# Import pre-built ggml library
add_library(ggml_lib SHARED IMPORTED)
set_target_properties(ggml_lib PROPERTIES
    IMPORTED_LOCATION "${LLAMA_CPP_INSTALL}/build_release/bin/libggml.so"
)

# Import pre-built llama.cpp libraries
add_library(llama_lib SHARED IMPORTED)

# Link to the specific release build folder
set_target_properties(llama_lib PROPERTIES
    IMPORTED_LOCATION "${LLAMA_CPP_INSTALL}/build_release/bin/libllama.so"
    INTERFACE_LINK_LIBRARIES "ggml_lib;ggml_base"
    # Add the ggml include path here so it propagates to anyone using llama_lib
    INTERFACE_INCLUDE_DIRECTORIES "${LLAMA_CPP_INSTALL}/include;${LLAMA_CPP_INSTALL}/ggml/include"
)

# command line parsing
add_library(command_parsing STATIC
    "${CMAKE_SOURCE_DIR}/../../src/commandline_args.cpp"
)
target_include_directories(command_parsing PUBLIC
    "${CMAKE_SOURCE_DIR}/../../include"
)

add_library(llm_wrapper STATIC
    ${CMAKE_SOURCE_DIR}/../src/llm_wrapper.cpp
)
target_include_directories(llm_wrapper PUBLIC
    "${LLAMA_CPP_INSTALL}/include"
    "${LLAMA_CPP_INSTALL}/ggml/include"
    "${LLAMA_CPP_INSTALL}/src"
    "${CMAKE_SOURCE_DIR}/../include"
)
target_link_libraries(llm_wrapper PRIVATE
    llama_lib
)

# add output executable
add_executable(llm_test ../src/main.cpp)

# Include directories for headers
target_include_directories(llm_test PRIVATE
    "${CMAKE_SOURCE_DIR}/../include"
)

# Link against the llama library
# llama.cpp defines the target 'llama'
target_link_libraries(llm_test PRIVATE 
    llm_wrapper
    command_parsing
)

# Set C++ standard (llama.cpp requires C++11 or higher, 17 is recommended)
target_compile_features(llm_test PRIVATE cxx_std_23)
