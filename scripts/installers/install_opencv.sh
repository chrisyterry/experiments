#!/bin/bash

echo "installing opencv"

# store the current script directory
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# install open cv
mkdir $SCRIPT_DIR/opencv_install
cd $SCRIPT_DIR/opencv_install

# Install minimal prerequisites (Ubuntu 18.04 as reference)
sudo apt update && sudo apt install -y cmake g++ wget unzip
 
# Download and unpack sources
wget -O opencv.zip https://github.com/opencv/opencv/archive/4.x.zip
unzip opencv.zip
 
# Create build directory
mkdir -p build && cd build
 
# Configure
cmake  ../opencv-4.x
 
# Build
cmake --build .

cd $SCRIPT_DIR
rm -r $SCRIPT_DIR/opencv_install