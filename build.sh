#!/bin/bash

# Create build directory
mkdir -p build

# Configure and build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DX86EMU_USE_86BOX=ON -DX86EMU_USE_MAME=ON ..
cmake --build . --parallel $(nproc)

echo "Build completed. Binaries are in build/bin"