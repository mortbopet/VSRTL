#!/bin/bash 
ls
cmake -DCOVERAGE=ON -DCMAKE_BUILD_TYPE=Debug .
cmake --build . --config Debug -- -j $(nproc)