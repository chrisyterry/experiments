#!/bin/bash

echo "Building llama-cpp"

# path to llama.cpp repo
llama_cpp_path=/home/chriz/Development/llama.cpp

# build llama.cpp in release
cmake -S $llama_cpp_path -B $llama_cpp_path/build_release \
    -DCMAKE_BUILD_TYPE=Release \
    -DGGML_CUDA=ON \
    -DCMAKE_CUDA_ARCHITECTURES="native"
cmake --build $llama_cpp_path/build_release --config Release -j7