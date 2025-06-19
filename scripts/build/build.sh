#!/bin/bash

# get the current script directory
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
start_dir=$PWD
# make a build directory within the build directory
mkdir "$SCRIPT_DIR/../../build/build"
cd "$SCRIPT_DIR/../../build/build"
cmake ..
make
cd $start_dir