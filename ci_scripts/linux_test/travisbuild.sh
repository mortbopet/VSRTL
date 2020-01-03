#!/bin/bash 
ls
cmake -DVSRTL_BUILD_APP=ON        \
    -DVSRTL_BUILD_TESTS=ON        \
    -DVSRTL_ENABLE_RISCV_TESTS=ON \
    -DVSRTL_COVERAGE=ON           \
    -DCMAKE_BUILD_TYPE=Debug      \
    .
cmake --build . --config Debug -- -j $(nproc)