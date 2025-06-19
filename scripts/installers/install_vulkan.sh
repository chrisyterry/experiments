#!/bin/bash

# installs the vulkan SDK to allow use of Vulkan for rendering and GPU compute

# command line utils
sudo apt install vulkan-tools 
# vulkan loader
sudo apt install libvulkan-dev 
# validation layers
sudo apt install spirv-tools
# below is workaround for ubuntu 24
wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-noble.list https://packages.lunarg.com/vulkan/lunarg-vulkan-noble.list
sudo apt update
sudo apt install vulkan-sdk

# GLFW window creation
sudo apt install libglfw3-dev
sudo apt install libxxf86vm-dev libxi-dev
# GLM  linear algebra for GPU
sudo apt install libglm-dev

# install curl
sudo apt install curl
# dowload the google gcc shader compiler (https://github.com/google/shaderc/blob/main/downloads.md?plain=1)
curl -o /tmp/google_shader_compiler.tgz -L https://storage.googleapis.com/shaderc/artifacts/prod/graphics_shader_compiler/shaderc/linux/continuous_gcc_release/494/20250423-103102/install.tgz
# install the shader compiler
tar -xvzf /tmp/google_shader_compiler.tgz -C /tmp 
sudo mv /tmp/install/bin/glslc /usr/local/bin/glslc
rm -r /tmp/install
rm /tmp/google_shader_compiler.tgz

# verify that vulkan is working
vkcube