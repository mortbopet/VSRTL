# VSRTL
[![Build Status](https://travis-ci.org/mortbopet/VSRTL.svg?branch=master)](https://travis-ci.org/mortbopet/VSRTL) [![codecov](https://codecov.io/gh/mortbopet/VSRTL/branch/master/graph/badge.svg)](https://codecov.io/gh/mortbopet/VSRTL)  
***V**isual **S**imulation of **R**egister **T**ransfer **L**ogic*

VSRTL is a collection of libraries and applications for visualizing simulations of digital circuits.
VSRTL is intended to be an intuitive HDL simulator which may be used for teaching digital circuits and boolean logic. 

Refer to the [reference section](docs/README.md) for implementation and usage documentation.  
If you would like to contribute, check the [issues](https://github.com/mortbopet/vsrtl/issues) section - There's plenty of work to be done!

<p align="center">
  <img src="https://github.com/mortbopet/vsrtl/blob/master/resources/gif1.gif?raw=true" width=75%/>
</p>

Figure: A simulation of a 8-bit counter using full adders. **Note**; placement and routing is being actively developed, and as such positioning of objects and wire routing are not final.

# Building
```
git clone --recurse-submodules https://github.com/mortbopet/VSRTL.git
cd VSRTL/
cmake .
make -j$(nproc)
```

## Dependencies:
* **Core**
  * C++17 toolchain
  * CMake
* **Graphics**
  * Qt 5.9.3+: https://www.qt.io/download
