#!/bin/bash

set -x
set -e

# this gets executed by Travis when building an App for Mac
# it gets started from inside the subsurface directory

export QT_ROOT=${TRAVIS_BUILD_DIR}/Qt/5.13.0/clang_64
export PATH=$QT_ROOT/bin:$PATH # Make sure correct qmake is found on the $PATH
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

ls
cmake .
make -j$(nproc)
