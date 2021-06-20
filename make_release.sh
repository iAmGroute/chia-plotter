#!/bin/bash -xe

mkdir -p build
cd build
cmake .. -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native -ffast-math" -DARITH="easy" -DBUILD_BLS_PYTHON_BINDINGS=false
cmake --build . --config Release -- -j4
