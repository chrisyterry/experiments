#!/bin/bash

function print_help_string_and_exit {
    echo "Usage: "$0" <build_mode>"
    echo " <build_mode> build mode to use, options are:"
    echo "  release - build release"
    echo "  debug - build debug"
    echo "  reldebinfo - release with debug info"
    exit 1
}

source 

# get the current script directory
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
start_dir=$PWD

build_mode="Release"

# for each input provided
while [[ $# -gt 0 ]]; do
case "$1" in
    -h--help) print_help_string_and_exit ;;
    release) build_mode="Release"; shift ;;
    debug) build_mode="Debug"; shift ;;
    reldebinfo) build_mode=RelWithDebInfo; shift ;;
    minsize) build_mode=MinSizeRel; shift ;;
    *) echo "Invalid argument $1"; print_help_string_and_exit
esac
done

echo "building $build_mode"

echo $SCRIPT_DIR

# make a build directory within the build directory
mkdir "$SCRIPT_DIR/../build/build"
cd "$SCRIPT_DIR/../build/build"
cmake -DCMAKE_BUILD_TYPE=$build_mode ..
make
cd $start_dir