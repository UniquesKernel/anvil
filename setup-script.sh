#!/bin/bash

# Update package lists
sudo apt-get update

# Install build essentials (gcc, make, etc)
sudo apt-get install -y build-essential clang-format

# Install CMake
sudo apt-get install -y cmake

# Install Python3, pip, and venv
sudo apt-get install -y python3 python3-pip python3-venv

# Install GDB and Valgrind
sudo apt-get install -y gdb valgrind

# Install Doxygen and related tools
sudo apt-get install -y doxygen doxygen-gui graphviz

# Check if we're in a virtual environment already
if [[ -z "${VIRTUAL_ENV}" ]]; then
  echo "No virtual environment detected, creating one..."
  # Create venv if it doesn't exist
  if [ ! -d "venv" ]; then
    python3 -m venv venv
  fi
  # Activate venv
  source venv/bin/activate
else
  echo "Using existing virtual environment: ${VIRTUAL_ENV}"
fi

# Install Python dependencies
pip install -r requirements.txt
