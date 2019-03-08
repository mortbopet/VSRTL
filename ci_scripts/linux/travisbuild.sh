#!/bin/bash 
ls
cmake .
make -j$(nproc)