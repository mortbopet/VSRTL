#!/bin/bash 
ls
cmake .
cmake --build . -- -j $(nproc)