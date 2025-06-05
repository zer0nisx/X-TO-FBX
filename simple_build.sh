#!/bin/bash

echo "Simple X2FBX Build Script"
echo "========================="

# Check if g++ is available
if ! command -v g++ &> /dev/null; then
    echo "Error: g++ compiler not found"
    echo "Please install build tools: apt install build-essential"
    exit 1
fi

# Create build directory
mkdir -p build

# Compile with basic flags
echo "Compiling X2FBX converter..."

g++ -std=c++17 -O2 \
    -DHAVE_ZLIB -DHAVE_BZIP2 \
    -I./include \
    -I./third_party/bzip2/include \
    -I./third_party/zlib \
    src/main.cpp \
    src/core/*.cpp \
    src/data/*.cpp \
    src/exporters/*.cpp \
    src/parsers/*.cpp \
    src/utils/*.cpp \
    -lz -lbz2 \
    -o build/x2fbx-converter

if [ $? -eq 0 ]; then
    echo "Build successful! Executable: build/x2fbx-converter"
else
    echo "Build failed!"
    exit 1
fi
