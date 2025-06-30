#!/bin/bash

# store the current script directory
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

sudo apt update

install_vscode=0
if [$install_vscode == 1]; then
    echo "installing VS Code"
    # general development
    sudo apt install code # VS code
fi
# PYTHON

# CONDA install
install_conda=0
if [$install_conda == 1]; then
    echo "installing Conda"
    conda_install_url=https://repo.anaconda.com/archive/Anaconda3-2024.10-1-Linux-x86_64.sh

    mkdir "./conda_install"
    cd "./conda_install"
    curl -O "$conda_install_url"
    filename="${conda_install_url##*/}"
    echo $filename
    /bin/bash "./$filename"
    cd $SCRIPT_DIR
    rm -r $conda_install
fi
# C++
echo "installing GCC"
sudo apt install build-essential # gnu C++ compiler
echo "installing CMake"
sudo apt install cmake # CMake
echo "installing Eigen"
sudo apt install libeigen3-dev # Eigen

# source library-specific installers; these will be prefixed with install_
for script in "$SCRIPT_DIR"/install_*.sh; do
  if [[ -f "$script" ]]; then #Check if it is a file
    # Source the script.
    source "$script"
    echo "Sourced: $script"
  else
    echo "Warning: $script not found."
  fi
done


# doxygen
sudo apt-add-repository universe
sudo apt-get update
sudo apt-get install doxygen