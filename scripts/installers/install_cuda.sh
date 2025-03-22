#!/bin/bash

# the following is pulled directy from Nvidia's website 
# https://docs.nvidia.com/cuda/cuda-installation-guide-linux/
# https://developer.nvidia.com/cuda-downloads?target_os=Linux&target_arch=x86_64&Distribution=Ubuntu&target_version=24.04&target_type=deb_local
# if the ubuntu version is ever changed, gonna need to modify this

echo "installing cuda"

# store the current script directory
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

mkdir $SCRIPT_DIR/cuda_install
cd cuda_install
echo "downloading first item..."
wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2404/x86_64/cuda-ubuntu2404.pin
sudo mv cuda-ubuntu2404.pin /etc/apt/preferences.d/cuda-repository-pin-600
echo "downloading second item..."
wget https://developer.download.nvidia.com/compute/cuda/12.8.1/local_installers/cuda-repo-ubuntu2404-12-8-local_12.8.1-570.124.06-1_amd64.deb
sudo dpkg -i cuda-repo-ubuntu2404-12-8-local_12.8.1-570.124.06-1_amd64.deb
sudo cp /var/cuda-repo-ubuntu2404-12-8-local/cuda-*-keyring.gpg /usr/share/keyrings/
sudo apt-get update
sudo apt-get -y install cuda-toolkit-12-8

# install nvidia driver
echo "installing cuda driver..."
sudo apt-get install -y nvidia-open

# add cuda environment variables to bash.rc
#PATH
cuda_path=/usr/local/cuda/bin
if grep -q "export PATH=\"\$PATH:$cuda_path\"" ~/.bashrc; then
  echo "Path '$cuda_path' is already in .bashrc; Skipping."
else
  # Append the new path to .bashrc.
  echo "export PATH=\"\$PATH:$cuda_path\"" >> ~/.bashrc
  echo "$cuda_path added to PATH in .bashrc."
fi
#LD_LIBRARY_PATH
cuda_ld_library_path=/usr/local/cuda/lib64
if grep -q "export LD_LIBRARY_PATH=\"\$LD_LIBRARY_PATH:$cuda_ld_library_path\"" ~/.bashrc; then
  echo "LD_LIBRARY_PATH '$cuda_ld_library_path' is already in .bashrc; Skipping."
else
  # Append the new path to .bashrc.
  echo "export LD_LIBRARY_PATH=\"\$LD_LIBRARY_PATH:$cuda_ld_library_path\"" >> ~/.bashrc
  echo "$cuda_ld_library_path added to LD_LIBRARY_PATH in .bashrc."
fi

# remove temp directory and return to start directory
cd $SCRIPT_DIR
rm -r $SCRIPT_DIR/cuda_install