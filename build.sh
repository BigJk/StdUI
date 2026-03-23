#!/bin/bash

mkdir build
cd build

GIT_TAG=$(git describe --tags)

echo "Building version $GIT_TAG"

if command -v clang >/dev/null 2>&1 && command -v clang++ >/dev/null 2>&1; then
    cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-DGIT_TAG=\\\"$GIT_TAG\\\"" ..
else
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-DGIT_TAG=\\\"$GIT_TAG\\\"" ..
fi

cmake --build .
