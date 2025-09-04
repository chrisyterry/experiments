#!/bin/bash

# store the current script directory
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

sudo add-apt-repository universe
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
sudo apt-get install -y build-essential cmake ninja-build # gnu C++ compiler
sudo apt install -y gcc-14 g++-14
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 13
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 14
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 13
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 14
echo "installing CMake"
sudo apt install -y cmake # CMake
echo "installing Eigen"
sudo apt install -y libeigen3-dev # Eigen
echo "installing clang"
sudo apt install -y clang # clang

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
sudo apt-get install -y doxygen

# various linux tools
sudo apt install -y tree
sudo apt install -y dust
sudo apt install -y openssh
sudo apt install -y rsync

# starship
curl -sS https://starship.rs/install.sh | sh
echo "eval '$(starship init bash)'" >> ~/.bashrc