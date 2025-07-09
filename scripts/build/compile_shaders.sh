#!/usr/bin/env bash
# compiles all shaders in the shaders directory

# get the script directory
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
SHADER_FOLDER_NAME=shaders
GLSLC_PATH=/usr/local/bin/glslc

# create a bin folder for the compiled shaders
sudo mkdir -p $SCRIPT_DIR/../../$SHADER_FOLDER_NAME/bin
sudo rm -rf $SCRIPT_DIR/../../$SHADER_FOLDER_NAME/bin/*

# for each shader in the shaders folder
for file in $SCRIPT_DIR/../../$SHADER_FOLDER_NAME/*; do 
    if [[ -f $file ]]; then 
        echo compiling $file
        # compile the shader
        file_name="${file%.*}"
        file_name="${file_name##*/}"
        # glslc can also compile shaders in human readable format for debugging
        $GLSLC_PATH $file -o $SCRIPT_DIR/../../$SHADER_FOLDER_NAME/bin/$file_name.spv
    fi 
done