#!/bin/bash

# Exit on error
set -e

# Create build directory
mkdir -p build
cd build

# Run CMake and build
cmake ..
make

echo "Build complete. Executables are in the 'build' directory."
