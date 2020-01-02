#!/bin/bash 
ls
cmake -DVSRTL_ENABLE_RISCV_TESTS=ON -DCOVERAGE=ON -DCMAKE_BUILD_TYPE=Debug .
cmake --build . --config Debug -- -j $(nproc)