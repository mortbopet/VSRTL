#!/bin/bash 
ls
cmake -DVSRTL_BUILD_APP=ON \
    -DVSRTL_BUILD_TESTS=ON \
    .
cmake --build . -- -j $(nproc)