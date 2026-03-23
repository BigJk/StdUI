#!/bin/bash

mkdir build_debug
cd build_debug
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ -DCMAKE_CXX_FLAGS="-fsanitize=address" -DCMAKE_C_FLAGS="-fsanitize=address" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address" ..
cmake --build .
